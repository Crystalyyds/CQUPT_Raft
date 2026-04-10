#include <gtest/gtest.h>

#include "raft/command.h"

namespace raftdemo {
namespace {

TEST(CommandTest, SetCommandSerializeAndDeserialize) {
  Command cmd;
  cmd.type = CommandType::kSet;
  cmd.key = "x";
  cmd.value = "100";

  EXPECT_TRUE(cmd.IsValid());
  EXPECT_EQ(cmd.Serialize(), "SET|x|100");

  Command parsed{};
  EXPECT_TRUE(Command::Deserialize(cmd.Serialize(), &parsed));
  EXPECT_EQ(parsed.type, CommandType::kSet);
  EXPECT_EQ(parsed.key, "x");
  EXPECT_EQ(parsed.value, "100");
}

TEST(CommandTest, DeleteCommandSerializeAndDeserialize) {
  Command cmd;
  cmd.type = CommandType::kDelete;
  cmd.key = "x";
  cmd.value = "ignored";

  EXPECT_TRUE(cmd.IsValid());
  EXPECT_EQ(cmd.Serialize(), "DEL|x|");

  Command parsed{};
  EXPECT_TRUE(Command::Deserialize(cmd.Serialize(), &parsed));
  EXPECT_EQ(parsed.type, CommandType::kDelete);
  EXPECT_EQ(parsed.key, "x");
  EXPECT_TRUE(parsed.value.empty());
}

TEST(CommandTest, EmptyKeyIsInvalid) {
  Command cmd;
  cmd.type = CommandType::kSet;
  cmd.key = "";
  cmd.value = "100";

  EXPECT_FALSE(cmd.IsValid());
}

TEST(CommandTest, UnknownCommandIsInvalid) {
  Command cmd;
  cmd.type = CommandType::kUnknown;
  cmd.key = "x";
  cmd.value = "100";

  EXPECT_FALSE(cmd.IsValid());
  EXPECT_TRUE(cmd.Serialize().empty());
}

TEST(CommandTest, DeserializeRejectsBadInput) {
  Command out{};

  EXPECT_FALSE(Command::Deserialize("", &out));
  EXPECT_FALSE(Command::Deserialize("SET|only_key", &out));
  EXPECT_FALSE(Command::Deserialize("UNKNOWN|x|1", &out));
  EXPECT_FALSE(Command::Deserialize("SET|", &out));
  EXPECT_FALSE(Command::Deserialize("DEL|", &out));
  EXPECT_FALSE(Command::Deserialize("SET|x|1", nullptr));
}

}  // namespace
}  // namespace raftdemo
