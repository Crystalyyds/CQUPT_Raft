#include <gtest/gtest.h>

#include <string>

#include "raft/command.h"
#include "raft/state_machine.h"

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

}  // namespace
}  // namespace raftdemo
