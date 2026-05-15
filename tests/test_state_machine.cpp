#include <gtest/gtest.h>

#include <string>

#include "raft/common/command.h"
#include "raft/state_machine/state_machine.h"

namespace raftdemo {
namespace {

Command MakeSet(const std::string& key, const std::string& value) {
  Command cmd;
  cmd.type = CommandType::kSet;
  cmd.key = key;
  cmd.value = value;
  return cmd;
}

Command MakeDelete(const std::string& key) {
  Command cmd;
  cmd.type = CommandType::kDelete;
  cmd.key = key;
  return cmd;
}

TEST(KvStateMachineTest, ApplySetThenReadBack) {
  KvStateMachine sm;

  ApplyResult result = sm.Apply(1, MakeSet("x", "1").Serialize());
  ASSERT_TRUE(result.Ok);

  std::string value;
  EXPECT_TRUE(sm.Get("x", &value));
  EXPECT_EQ(value, "1");
}

TEST(KvStateMachineTest, ApplySetCanOverwriteOldValue) {
  KvStateMachine sm;

  ASSERT_TRUE(sm.Apply(1, MakeSet("x", "1").Serialize()).Ok);
  ASSERT_TRUE(sm.Apply(2, MakeSet("x", "100").Serialize()).Ok);

  std::string value;
  EXPECT_TRUE(sm.Get("x", &value));
  EXPECT_EQ(value, "100");
}

TEST(KvStateMachineTest, ApplyDeleteRemovesExistingKey) {
  KvStateMachine sm;

  ASSERT_TRUE(sm.Apply(1, MakeSet("x", "1").Serialize()).Ok);
  ASSERT_TRUE(sm.Apply(2, MakeDelete("x").Serialize()).Ok);

  std::string value;
  EXPECT_FALSE(sm.Get("x", &value));
}

TEST(KvStateMachineTest, DeleteMissingKeyStillSucceeds) {
  KvStateMachine sm;

  ApplyResult result = sm.Apply(1, MakeDelete("not_exist").Serialize());
  EXPECT_TRUE(result.Ok);
}

TEST(KvStateMachineTest, InvalidCommandDataReturnsFailure) {
  KvStateMachine sm;

  ApplyResult result = sm.Apply(1, "BAD|command");
  EXPECT_FALSE(result.Ok);
}

TEST(KvStateMachineTest, DebugStringShowsSortedKeys) {
  KvStateMachine sm;

  ASSERT_TRUE(sm.Apply(1, MakeSet("b", "2").Serialize()).Ok);
  ASSERT_TRUE(sm.Apply(2, MakeSet("a", "1").Serialize()).Ok);
  ASSERT_TRUE(sm.Apply(3, MakeSet("c", "3").Serialize()).Ok);

  EXPECT_EQ(sm.DebugString(), "{a=1, b=2, c=3}");
}

TEST(KvStateMachineTest, ReplayStyleOverwriteAndDeleteLeaveExpectedFinalView) {
  KvStateMachine sm;

  ASSERT_TRUE(sm.Apply(1, MakeSet("snapshot_only", "base").Serialize()).Ok);
  ASSERT_TRUE(sm.Apply(2, MakeSet("tail_overwrite", "before").Serialize()).Ok);
  ASSERT_TRUE(sm.Apply(3, MakeSet("tail_delete", "present").Serialize()).Ok);

  ASSERT_TRUE(sm.Apply(4, MakeSet("tail_overwrite", "after").Serialize()).Ok);
  ASSERT_TRUE(sm.Apply(5, MakeDelete("tail_delete").Serialize()).Ok);
  ASSERT_TRUE(sm.Apply(6, MakeSet("tail_only", "replayed").Serialize()).Ok);

  std::string value;
  EXPECT_TRUE(sm.Get("snapshot_only", &value));
  EXPECT_EQ(value, "base");

  EXPECT_TRUE(sm.Get("tail_overwrite", &value));
  EXPECT_EQ(value, "after");

  EXPECT_FALSE(sm.Get("tail_delete", &value));

  EXPECT_TRUE(sm.Get("tail_only", &value));
  EXPECT_EQ(value, "replayed");

  EXPECT_EQ(sm.DebugString(),
            "{snapshot_only=base, tail_only=replayed, tail_overwrite=after}");
}

}  // namespace
}  // namespace raftdemo
