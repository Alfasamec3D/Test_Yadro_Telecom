#include "Simulator.hpp"

#include <gtest/gtest.h>
using namespace Simulator;

// Dummy bot
class ScriptedBot : public Bot::IBot {
 public:
  explicit ScriptedBot(std::vector<Bot::Action> script)
      : script_(std::move(script)) {}

  Bot::Action decide(const Bot::BotView&) override {
    if (idx_ >= script_.size()) return Bot::Action{Bot::ACT_STOP, 0, RES_IRON};
    return script_[idx_++];
  }

 private:
  std::vector<Bot::Action> script_;
  size_t idx_ = 0;
};

TEST(RunSimulationTest, BasicCase) {
  std::vector<size_t> n0 = {1};
  std::map<ResourceType, int> r0 = {
      {RES_IRON, 0}, {RES_GOLD, 0}, {RES_GEMS, 0}, {RES_EXP, 0}};

  std::vector<size_t> n1 = {0, 2};
  std::map<ResourceType, int> r1 = {
      {RES_IRON, 1}, {RES_GOLD, 2}, {RES_GEMS, 0}, {RES_EXP, 0}};

  std::vector<size_t> n2 = {};
  std::map<ResourceType, int> r2 = {
      {RES_IRON, 0}, {RES_GOLD, 0}, {RES_GEMS, 1}, {RES_EXP, 0}};

  std::vector<Terracraft::Room> rooms;
  rooms.emplace_back(0u, n0.begin(), n0.end(), r0);
  rooms.emplace_back(1u, n1.begin(), n1.end(), r1);
  rooms.emplace_back(2u, n2.begin(), n2.end(), r2);

  Terracraft::Dungeon d(RES_GOLD, rooms.begin(), rooms.end());

  ScriptedBot bot({
      {Bot::ACT_MOVE, 1, RES_IRON},
      {Bot::ACT_COLLECT, 0, RES_GOLD},
      {Bot::ACT_COLLECT, 0, RES_IRON},
      {Bot::ACT_MOVE, 2, RES_IRON},
      {Bot::ACT_COLLECT, 0, RES_GEMS},
      {Bot::ACT_MOVE, 1, RES_IRON},
      {Bot::ACT_MOVE, 0, RES_IRON},
  });

  std::ostringstream out;
  run_simulation(d, 5, bot, out);

  const std::string expected =
      "go 1\n"
      "state 1 1 2 0 0\n"
      "collect gold\n"
      "state 1 1 _ 0 0\n"
      "collect iron\n"
      "state 1 _ _ 0 0\n"
      "go 2\n"
      "state 2 0 0 1 0\n"
      "collect gems\n"
      "state 2 0 0 _ 0\n"
      "go 1\n"
      "state 1 _ _ 0 0\n"
      "go 0\n"
      "result 1 2 1 0 74\n";

  EXPECT_EQ(out.str(), expected);
}
