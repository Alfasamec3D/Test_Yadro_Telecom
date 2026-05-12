#include <fstream>
#include <iostream>

#include "AliceBot.hpp"
#include "Parser.hpp"
#include "Simulator.hpp"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: task <input.txt>\n";
    return 1;
  }

  ParseResult pr = load_dungeon(argv[1]);

  std::ofstream fout("result.txt");
  if (!pr.ok) {
    fout << pr.bad_line << "\n";
    return 0;
  }

  // Чтобы испытать другой алгоритм — замените тип здесь:
  AliceBot bot;
  run_simulation(*pr.dungeon, pr.food, bot, fout);
  return 0;
}