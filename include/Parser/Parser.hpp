#pragma once

#include <memory>
#include <string>

#include "Dungeon.hpp"

struct ParseResult {
  bool ok;
  std::string bad_line;  // строка с ошибкой (как во входном файле)

  // Заполнено при ok==true. unique_ptr, потому что Dungeon содержит const
  // поля и не имеет operator= — мы не можем сделать optional<Dungeon> или
  // присваивать Dungeon после конструирования.
  std::unique_ptr<Dungeon> dungeon;

  // Параметр персонажа на этот запуск. Парсер кладёт сюда значение M из
  // последней строки. Не часть Dungeon — это параметр игрока.
  int food = 0;
};

// Читает подземелье из файла. При ошибке заполняет bad_line.
ParseResult load_dungeon(const std::string& path);