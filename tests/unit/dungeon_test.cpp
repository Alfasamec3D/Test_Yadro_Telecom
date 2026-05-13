#include "Dungeon.hpp"

#include <gtest/gtest.h>
using namespace Terracraft;

TEST(GrabTest, BasicCase) {
  const size_t id = 1;
  const std::vector<size_t> neigh = {1, 2, 3, 4};

  std::map<ResourceType, int> expected_res = {
      {RES_IRON, 2}, {RES_GOLD, 5}, {RES_GEMS, 7}, {RES_EXP, 11}};

  Room room{id, neigh.begin(), neigh.end(), expected_res};

  const ResourceType& gr_type = RES_GOLD;
  const std::set<ResourceType>& expected_gr_res = {gr_type};
  const int expected_gr = expected_res[gr_type];
  expected_res[gr_type] = 0;

  const int gr = room.grab(gr_type);
  const std::map<ResourceType, int> res = room.resources();
  const std::set<ResourceType>& gr_res = room.grabbed_resources();

  EXPECT_EQ(gr, expected_gr);
  EXPECT_EQ(res, expected_res);
  EXPECT_EQ(gr_res, expected_gr_res);
}

TEST(OstreamTest, BasicCase) {
  const size_t id = 1;
  const std::vector<size_t> neigh = {1, 2, 3, 4};

  std::map<ResourceType, int> expected_res = {
      {RES_IRON, 2}, {RES_GOLD, 5}, {RES_GEMS, 7}, {RES_EXP, 11}};

  Room room{id, neigh.begin(), neigh.end(), expected_res};

  std::ostringstream os;
  os << room;
  const std::string expected_os = "state 1 2 5 7 11";
  EXPECT_EQ(os.str(), expected_os);
}