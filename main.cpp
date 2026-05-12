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

  AliceBot bot;
  run_simulation(*pr.dungeon, pr.M_, bot, fout);
  return 0;
}