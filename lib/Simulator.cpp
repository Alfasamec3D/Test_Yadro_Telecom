#include "Simulator.hpp"

#include <algorithm>
#include <map>
#include <ostream>

namespace {

// Update knowledge of the visited room
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

int compute_total_value(const std::map<ResourceType, int>& totals,
                        const ResourceType& target) {
  int val{};
  for (const auto& [type, count] : totals) {
    if (type == target)
      val += 2 * count * BASE_VALUES.at(type);
    else
      val += count * BASE_VALUES.at(type);
  }
  return val;
}

void print_result_line(std::ostream& out,
                       const std::map<ResourceType, int>& totals, int val) {
  out << "result";
  for (const auto& [_, count] : totals) {
    out << ' ' << count;
  }
  out << ' ' << val << std::endl;
}

}  // namespace

void run_simulation(Dungeon& d, int food, IBot& bot, std::ostream& out) {
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

  std::map<ResourceType, int> totals;
  for (const auto& [type, _] : BASE_VALUES) totals[type] = 0;

  reveal_on_enter(v, d, 0);

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
      if (to != 0) out << d.room_id(to) << std::endl;
      if (to == 0) {
        int val = compute_total_value(totals, d.target());
        print_result_line(out, totals, val);
        return;
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

      if (!d.room_id(room).grabbed_resources().empty()) {
        if (v.food_left <= 0) break;
        v.food_left -= 1;
      }

      totals[k] += d.room_id(room).grab(k);

      it->second = 0;

      out << "collect " << RES_NAMES.at(k) << std::endl
          << d.room_id(room) << std::endl;
      continue;
    }
    break;
  }

  if (v.current == 0)
    print_result_line(out, totals, compute_total_value(totals, d.target()));
  return;
}