#include "Simulator.hpp"

#include <algorithm>
#include <map>
#include <ostream>

namespace {



// Обновляет известность для комнаты, в которую бот «зашёл».
void reveal_on_enter(BotView& v, const Dungeon& d, int room) {
  auto bump = [&](int x, Knowledge target) {
    if (v.known[x] < target) v.known[x] = target;
  };
  auto fill_neighbors = [&](int x) {
    if (v.neighbors[x].empty() && !d.room_id(x).neighbors().empty()) {
      v.neighbors[x] = d.room_id(x).neighbors();
    }
  };

  bump(room, VISITED);
  v.res[room] = d.room_id(room).resources();
  fill_neighbors(room);

  for (int nb : d.room_id(room).neighbors()) {
    bump(nb, VISIBLE);
    fill_neighbors(nb);
    for (int nn : d.room_id(nb).neighbors()) {
      bump(nn, NUMBERED);
    }
  }
}

long long compute_total_value(const std::map<ResourceType, long long>& totals,
                              ResourceType target) {
  long long val = 0;
  for (const auto& [type, count] : totals) {
    auto it = BASE_VALUES.find(type);
    if (it == BASE_VALUES.end()) continue;
    long long base = it->second;
    if (type == target) base *= 2;
    val += base * count;
  }
  return val;
}

void print_result_line(std::ostream& out,
                       const std::map<ResourceType, long long>& totals,
                       long long val) {
  out << "result";
  for (const auto& [type, count] : totals) {
    (void)type;
    out << ' ' << count;
  }
  out << ' ' << val << '\n';
}

}  // namespace

long long run_simulation(Dungeon& d, int food, IBot& bot, std::ostream& out) {
  const int n = d.N();

  BotView v;
  v.n = n;
  v.current = 0;
  v.food_left = food;
  v.food_total = food;
  v.target = d.target();
  v.known.assign(n + 1, UNKNOWN);
  v.neighbors.assign(n + 1, {});
  v.res.assign(n + 1, {});

  std::map<ResourceType, long long> totals;
  for (const auto& [type, _] : BASE_VALUES) totals[type] = 0;

  reveal_on_enter(v, d, 0);

  auto print_state_now = [&](int room) {
    out << "state " << room;
    // Состояние ресурса — _ если он есть в grabbed_resources комнаты.
    // grabbed_resources на Room — источник истины.
    const auto& grabbed = d.room_id(room).grabbed_resources();
    for (const auto& [type, count] : v.res[room]) {
      out << ' ';
      if (grabbed.count(type))
        out << '_';
      else
        out << count;
    }
    out << '\n';
  };

  int safety_limit = 100000;
  while (safety_limit-- > 0) {
    Action a = bot.decide(v);

    if (a.kind == ACT_STOP) break;

    if (a.kind == ACT_MOVE) {
      int to = a.target_room;
      const auto& nb_cur = v.neighbors[v.current];
      bool ok_edge =
          std::find(nb_cur.begin(), nb_cur.end(), to) != nb_cur.end();
      if (!ok_edge) {
        const auto& nb_to = v.neighbors[to];
        ok_edge =
            std::find(nb_to.begin(), nb_to.end(), v.current) != nb_to.end();
      }
      if (!ok_edge) break;
      if (v.food_left <= 0 && v.current != 0) break;
      v.food_left -= 1;
      v.current = to;
      reveal_on_enter(v, d, to);
      out << "go " << to << '\n';
      if (to != 0) print_state_now(to);
      if (to == 0) {
        long long val = compute_total_value(totals, d.target());
        print_result_line(out, totals, val);
        return val;
      }
      continue;
    }

    if (a.kind == ACT_COLLECT) {
      int room = v.current;
      ResourceType k = a.resource;
      auto it = v.res[room].find(k);
      if (it == v.res[room].end() || it->second <= 0) break;

      // Источник истины — Room. Первый сбор в комнате (когда grabbed
      // ещё пуст) бесплатный, последующие стоят 1 еды.
      bool any_before = !d.room_id(room).grabbed_resources().empty();
      if (any_before) {
        if (v.food_left <= 0) break;
        v.food_left -= 1;
      }

      // grab() возвращает забранное количество и мутирует Room.
      int amount = d.room_id(room).grab(k);
      totals[k] += amount;
      // Синхронизируем то, что бот «видит» в этой комнате.
      it->second = 0;

      auto name_it = RES_NAMES.find(k);
      out << "collect "
          << (name_it != RES_NAMES.end() ? name_it->second : std::string("?"))
          << '\n';
      print_state_now(room);
      continue;
    }
    break;
  }

  if (v.current == 0) {
    long long val = compute_total_value(totals, d.target());
    print_result_line(out, totals, val);
    return val;
  }
  return 0;
}