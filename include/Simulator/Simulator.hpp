#pragma once

#include <array>
#include <iosfwd>
#include <vector>

#include "Dungeon.hpp"
#include "IBot.hpp"

// Запускает симуляцию: пишет в out строки go/state/collect/result.
// Возвращает итоговую ценность.
long long run_simulation(const Dungeon& d, IBot& bot, std::ostream& out);