#pragma once

#include <iosfwd>

#include "Dungeon.hpp"
#include "IBot.hpp"

void run_simulation(Dungeon& d, int food, IBot& bot, std::ostream& out);