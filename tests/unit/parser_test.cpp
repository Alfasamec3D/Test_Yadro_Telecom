#include "Parser.hpp"

#include <gtest/gtest.h>

#include <filesystem>
using namespace Parser;

TEST(Load_Dungeon_Test, BasicCase) {
  const std::filesystem::path path =
      std::filesystem::path(TEST_DATA_DIR) / "002" / "in.txt";

  ParseResult expected_parse;
  expected_parse.ok = false;
  expected_parse.M_ = 6;
  expected_parse.bad_line = "4 2 | 5 2 4 0 15";

  ParseResult parse = load_dungeon(path.string());

  EXPECT_EQ(parse.ok, expected_parse.ok);
  EXPECT_EQ(parse.bad_line, expected_parse.bad_line);
}
