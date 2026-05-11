#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Resources.hpp"

struct Room {
  int id;
  std::vector<int> neighbors;
  int resources[RES_COUNT];  // iron, gold, gems, exp
};

struct Dungeon {
  int n;                    // количество комнат, не считая нулевую
  std::vector<Room> rooms;  // размер n+1, индекс = номер комнаты
  int food;                 // M
  ResourceType target;      // целевой ресурс с удвоенной ценностью
};

struct ParseResult {
  bool ok;
  std::string bad_line;  // строка с ошибкой (как во входном файле)
};

// Читает подземелье из файла. При ошибке заполняет bad_line.
ParseResult load_dungeon(const std::string& path, Dungeon& out);