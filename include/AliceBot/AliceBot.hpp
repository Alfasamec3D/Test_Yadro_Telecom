#pragma once

#include "IBot.hpp"
#include <vector>

class AliceBot : public IBot {
public:
    Action decide(const BotView& v) override;

private:
    // Когда мы решили возвращаться — фиксируем путь, чтобы соблюсти
    // правило «на каждой развилке — наименьший номер» и идти строго им.
    std::vector<int> return_path_;   // последовательность комнат, КУДА идти
    int return_idx_ = 0;
    bool returning_ = false;

    // Помним, в каких комнатах уже делали «первый сбор» в фазе исследования.
    // Размер фиксируем при первом вызове decide.
    std::vector<bool> collected_here_;
    void ensure_sized(int n) {
        if ((int)collected_here_.size() < n + 1) collected_here_.assign(n + 1, false);
    }
};
