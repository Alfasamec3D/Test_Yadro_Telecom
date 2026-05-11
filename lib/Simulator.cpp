#include "Simulator.hpp"

#include <algorithm>
#include <ostream>
#include <set>

namespace {

// Обновляет известность для комнаты, в которую бот «зашёл».
// По правилам:
//   - сама комната становится VISITED (ресурсы открыты).
//   - каждый её сосед становится не ниже VISIBLE (видны его проходы).
//   - каждый сосед соседа — не ниже NUMBERED (известен номер).
void reveal_on_enter(BotView& v, const Dungeon& d, int room) {
  auto bump = [&](int x, Knowledge target) {
    if (v.known[x] < target) v.known[x] = target;
  };
  auto fill_neighbors = [&](int x) {
    if (v.neighbors[x].empty() && !d.rooms[x].neighbors.empty()) {
      v.neighbors[x] = d.rooms[x].neighbors;
    }
  };

  bump(room, VISITED);
  // ресурсы открываются
  for (int k = 0; k < RES_COUNT; ++k)
    v.res[room][k] = d.rooms[room].resources[k];
  fill_neighbors(room);

  for (int nb : d.rooms[room].neighbors) {
    bump(nb, VISIBLE);
    fill_neighbors(nb);
    for (int nn : d.rooms[nb].neighbors) {
      bump(nn, NUMBERED);
      // У NUMBERED видим только номер — проходы НЕ заполняем.
    }
  }
}

}  // namespace

long long run_simulation(const Dungeon& d, IBot& bot, std::ostream& out) {
  BotView v;
  v.n = d.n;
  v.current = 0;
  v.food_left = d.food;
  v.food_total = d.food;
  v.target = d.target;
  v.known.assign(d.n + 1, UNKNOWN);
  v.neighbors.assign(d.n + 1, {});
  v.res.assign(d.n + 1, {-1, -1, -1, -1});

  // Помечаем «было ли что собирать» — для печати "_".
  // Это отдельно от текущих res[], потому что сборку трактуем
  // как «забрать всё»: после сбора res[room][k] = 0, а флаг "был сбор" — true.
  std::vector<std::array<bool, RES_COUNT>> collected(
      d.n + 1, {false, false, false, false});

  // Итог.
  long long totals[RES_COUNT] = {0, 0, 0, 0};

  // Заходим в комнату 0.
  reveal_on_enter(v, d, 0);

  auto print_state_now = [&](int room) {
    out << "state " << room;
    for (int k = 0; k < RES_COUNT; ++k) {
      out << ' ';
      if (collected[room][k])
        out << '_';
      else
        out << v.res[room][k];
    }
    out << '\n';
  };

  // Главный цикл: бот сам решает, когда остановиться (ACT_STOP),
  // но симулятор страхует от безумных решений (нет еды и не дома и т.п.).
  int safety_limit = 100000;
  while (safety_limit-- > 0) {
    Action a = bot.decide(v);

    if (a.kind == ACT_STOP) {
      break;
    }

    if (a.kind == ACT_MOVE) {
      int to = a.target_room;
      // Рёбра считаем неориентированными: разрешён переход, если to
      // есть в списке соседей current ИЛИ current есть в списке to.
      // (В примере подземелья списки соседей могут быть «несогласованы»,
      // но реальная проходимость — в обе стороны.)
      const auto& nb_cur = v.neighbors[v.current];
      bool ok_edge =
          std::find(nb_cur.begin(), nb_cur.end(), to) != nb_cur.end();
      if (!ok_edge) {
        const auto& nb_to = v.neighbors[to];
        ok_edge =
            std::find(nb_to.begin(), nb_to.end(), v.current) != nb_to.end();
      }
      if (!ok_edge) break;
      // Тратим еду.
      if (v.food_left <= 0 && v.current != 0) break;
      v.food_left -= 1;
      v.current = to;
      reveal_on_enter(v, d, to);
      out << "go " << to << '\n';
      // По условию state выводится после каждого действия, кроме
      // возврата в стартовую комнату. Поэтому при возврате в 0 — нет state.
      if (to != 0) print_state_now(to);
      // Если пришли в стартовую — это финал.
      if (to == 0) {
        // Печатаем result.
        out << "result";
        for (int k = 0; k < RES_COUNT; ++k) out << ' ' << totals[k];
        long long val = 0;
        for (int k = 0; k < RES_COUNT; ++k) {
          long long base = BASE_VALUES[k];
          if ((ResourceType)k == d.target) base *= 2;
          val += base * totals[k];
        }
        out << ' ' << val << '\n';
        return val;
      }
      continue;
    }

    if (a.kind == ACT_COLLECT) {
      int room = v.current;
      ResourceType k = a.resource;
      int amount = v.res[room][k];
      if (amount <= 0) break;
      // Стоимость: если это не «первый сбор в комнате», тратится 1 еды.
      bool any_collected_before = false;
      for (int j = 0; j < RES_COUNT; ++j)
        if (collected[room][j]) {
          any_collected_before = true;
          break;
        }
      if (any_collected_before) {
        if (v.food_left <= 0) break;
        v.food_left -= 1;
      }
      // Забираем всё.
      totals[k] += amount;
      v.res[room][k] = 0;
      collected[room][k] = true;
      out << "collect " << RES_NAMES[k] << '\n';
      print_state_now(room);
      continue;
    }
    break;
  }

  // Если вышли из цикла «штатно» через ACT_STOP — значит бот сам решил
  // закончить. Симулятор требует, чтобы финал был в комнате 0 и printed result.
  // Если бот этого не достиг, всё равно выведем result — но это будет
  // означать ошибку алгоритма (персонаж умер / застрял).
  if (v.current == 0) {
    out << "result";
    for (int k = 0; k < RES_COUNT; ++k) out << ' ' << totals[k];
    long long val = 0;
    for (int k = 0; k < RES_COUNT; ++k) {
      long long base = BASE_VALUES[k];
      if ((ResourceType)k == d.target) base *= 2;
      val += base * totals[k];
    }
    out << ' ' << val << '\n';
    return val;
  }
  return 0;  // персонаж умер
}