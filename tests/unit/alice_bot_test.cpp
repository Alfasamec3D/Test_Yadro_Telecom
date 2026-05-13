#include "AliceBot.hpp"

#include <gtest/gtest.h>
using namespace Bot;

TEST(DecideTest, BasicCase) {
  BotView v;
  v.current = 1;
  v.food_total = 10;
  v.food_left = 9;
  v.target = RES_EXP;
 
  v.known[0] = VISITED;
  v.known[1] = VISITED;
  v.neighbors[0] = {1};
  v.neighbors[1] = {0};
 
  v.res[0] = {{RES_IRON, 0}, {RES_GOLD, 0}, {RES_GEMS, 0}, {RES_EXP, 0}};
  v.res[1] = {{RES_IRON, 10}, {RES_GOLD, 2}, {RES_GEMS, 0}, {RES_EXP, 5}};
 
  AliceBot bot;
  Action a = bot.decide(v);
 
  EXPECT_EQ(a.kind, ACT_COLLECT);
  EXPECT_EQ(a.resource, RES_GOLD);
}
