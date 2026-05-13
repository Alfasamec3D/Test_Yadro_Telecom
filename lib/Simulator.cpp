#include "Simulator.hpp"

#include <algorithm>
#include <map>
#include <ostream>

namespace Simulator {
namespace {

void reveal_on_enter(Bot::BotView& v, const Terracraft::Dungeon& d, int room) {
  auto bump = [&](int x, Bot::Knowledge target) {
    auto it = v.known.find(x);
    if (it == v.known.end() || it->second < target) v.known[x] = target;
  };
  // Кладём ребро (a, b) с обеих сторон в v.neighbors. Дубликаты не плодим.
  auto add_edge = [&](int a, int b) {
    auto& la = v.neighbors[a];
    if (std::find(la.begin(), la.end(), b) == la.end()) la.push_back(b);
    auto& lb = v.neighbors[b];
    if (std::find(lb.begin(), lb.end(), a) == lb.end()) lb.push_back(a);
  };
  auto fill_neighbors = [&](int x) {
    for (int y : d.room_id(x).neighbors()) add_edge(x, y);
  };

  bump(room, Bot::VISITED);
  v.res[room] = d.room_id(room).resources();
  fill_neighbors(room);

  for (int nb : d.room_id(room).neighbors()) {
    bump(nb, Bot::VISIBLE);
    fill_neighbors(nb);
    for (int nn : d.room_id(nb).neighbors()) {
      bump(nn, Bot::NUMBERED);
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

const std::vector<int>& neighbors_of(const Bot::BotView& v, int room) {
  static const std::vector<int> empty;
  auto it = v.neighbors.find(room);
  return it == v.neighbors.end() ? empty : it->second;
}

}  // namespace

void run_simulation(Terracraft::Dungeon& d, int food, Bot::IBot& bot,
                    std::ostream& out) {
  Bot::BotView v;
  v.current = 0;
  v.food_left = food;
  v.food_total = food;
  v.target = d.target();
  // known/neighbors/res are fiiles via reveal_on_enter

  std::map<ResourceType, int> totals;
  for (const auto& [type, _] : BASE_VALUES) totals[type] = 0;

  reveal_on_enter(v, d, 0);

  int safety_limit = 100000;
  while (safety_limit-- > 0) {
    Bot::Action a = bot.decide(v);

    if (a.kind == Bot::ACT_STOP) break;

    if (a.kind == Bot::ACT_MOVE) {
      int to = a.target_room;
      const auto& nb_cur = neighbors_of(v, v.current);
      bool ok_edge =
          std::find(nb_cur.begin(), nb_cur.end(), to) != nb_cur.end();
      if (!ok_edge) {
        const auto& nb_to = neighbors_of(v, to);
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

    if (a.kind == Bot::ACT_COLLECT) {
      int room = v.current;
      ResourceType k = a.resource;
      auto& room_res = v.res[room];
      auto it = room_res.find(k);
      if (it == room_res.end() || it->second <= 0) break;

      // Source of true is room. Fiesr grab when grabbed is empty is free
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
}

}  // namespace Simulator