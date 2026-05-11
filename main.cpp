#include "Simulator.hpp"
#include "AliceBot.hpp"

#include <fstream>
#include <iostream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: task <input.txt>\n";
        return 1;
    }

    Dungeon d;
    ParseResult pr = load_dungeon(argv[1], d);

    std::ofstream fout("result.txt");
    if (!pr.ok) {
        fout << pr.bad_line << "\n";
        return 0;
    }

    // Чтобы испытать другой алгоритм — замените тип здесь:
    AliceBot bot;
    run_simulation(d, bot, fout);
    return 0;
}