#pragma once
#include <array>
#include <vector>

#include "Resources.hpp"
// Что бот знает про каждую комнату — см. правила раскрытия в условии.
enum Knowledge {
  UNKNOWN = 0,   // вообще не известна
  NUMBERED = 1,  // известен только номер
  VISIBLE = 2,   // видны проходы из неё, но не ресурсы
  VISITED = 3    // полная информация
};

// Снимок мира с точки зрения бота. Симулятор обновляет его сам.
// Бот принимает решения ТОЛЬКО на основе этого снимка — это
// соответствует правилам раскрытия карты из условия.
struct BotView {
  int n;
  int current;
  int food_left;
  int food_total;  // M, нужен для фазы
  ResourceType target;
  std::vector<Knowledge> known;
  std::vector<std::vector<int>> neighbors;  // непусто для NUMBERED+
  std::vector<std::array<int, RES_COUNT>>
      res;  // -1 в каждой позиции, если не VISITED
};

enum ActionKind { ACT_MOVE, ACT_COLLECT, ACT_STOP };
struct Action {
  ActionKind kind;
  int target_room;
  ResourceType resource;
};

class IBot {
 public:
  virtual ~IBot() = default;
  virtual Action decide(const BotView& v) = 0;
};