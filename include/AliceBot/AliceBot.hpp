#pragma once

#include <vector>

#include "IBot.hpp"
namespace Bot {
class AliceBot final : public IBot {
 public:
  Action decide(const BotView& v) override;

 private:
  // when we dicside to retunr we log the path to choose least number on fork
  std::vector<int> return_path_;
  int return_idx_ = 0;
  bool returning_ = false;

  std::vector<bool> collected_here_;
  void ensure_sized(int room) {
    if ((int)collected_here_.size() <= room)
      collected_here_.resize(room + 1, false);
  }
};
}  // namespace Bot