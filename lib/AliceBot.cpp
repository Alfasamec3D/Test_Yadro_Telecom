#include "AliceBot.hpp"

#include <algorithm>
#include <climits>
#include <map>
#include <queue>
namespace Bot {
namespace {

int known_level(const BotView& v, int room) {
  auto it = v.known.find(room);
  return it == v.known.end() ? 0 : (int)it->second;
}

const std::vector<int>& neighbors_of(const BotView& v, int room) {
  static const std::vector<int> empty;
  auto it = v.neighbors.find(room);
  return it == v.neighbors.end() ? empty : it->second;
}

const std::map<ResourceType, int>& res_of(const BotView& v, int room) {
  static const std::map<ResourceType, int> empty;
  auto it = v.res.find(room);
  return it == v.res.end() ? empty : it->second;
}

// Distance from from to 0 through  VISITED rooms
int distance_to_zero_visited(const BotView& v, int from) {
  if (from == 0) return 0;
  std::map<int, int> dist;
  std::queue<int> q;
  q.push(from);
  dist[from] = 0;
  while (!q.empty()) {
    int u = q.front();
    q.pop();
    for (int nb : neighbors_of(v, u)) {
      if (known_level(v, nb) != VISITED) continue;
      if (dist.count(nb)) continue;
      dist[nb] = dist[u] + 1;
      if (nb == 0) return dist[nb];
      q.push(nb);
    }
  }
  return INT_MAX;
}

std::vector<int> shortest_path_to_zero(const BotView& v, int from) {
  std::map<int, int> dist;
  std::queue<int> q;
  dist[0] = 0;
  q.push(0);
  while (!q.empty()) {
    int u = q.front();
    q.pop();
    for (int nb : neighbors_of(v, u)) {
      if (known_level(v, nb) != VISITED) continue;
      if (dist.count(nb)) continue;
      dist[nb] = dist[u] + 1;
      q.push(nb);
    }
  }
  std::vector<int> path;
  if (!dist.count(from)) return path;
  int cur = from;
  while (cur != 0) {
    int best = -1;
    for (int nb : neighbors_of(v, cur)) {
      auto it = dist.find(nb);
      if (it == dist.end() || it->second != dist[cur] - 1) continue;
      if (best == -1 || nb < best) best = nb;
    }
    if (best == -1) return {};
    path.push_back(best);
    cur = best;
  }
  return path;
}

struct BestResult {
  bool has;
  ResourceType type;
};

BestResult best_resource_in_room(const BotView& v, int room) {
  BestResult result{false, RES_IRON};
  long long best_val = -1;
  for (const auto& [type, count] : res_of(v, room)) {
    if (count <= 0) continue;
    auto base_it = BASE_VALUES.find(type);
    if (base_it == BASE_VALUES.end()) continue;
    long long val = base_it->second;
    if (type == v.target) val *= 2;
    if (val > best_val) {
      best_val = val;
      result = {true, type};
    }
  }
  return result;
}

// next step while investigating
int next_explore_target(const BotView& v) {
  int cur = v.current;
  // Negihbor not visited but known at some rate are th least number
  int best = -1;
  for (int nb : neighbors_of(v, cur)) {
    int lvl = known_level(v, nb);
    if (lvl == 0 || lvl == VISITED) continue;
    if (best == -1 || nb < best) best = nb;
  }
  if (best != -1) return best;

  // Search nearest not visited known
  std::map<int, int> dist;
  std::queue<int> q;
  dist[cur] = 0;
  q.push(cur);
  int best_dist = INT_MAX, best_room = -1;
  while (!q.empty()) {
    int u = q.front();
    q.pop();
    for (int nb : neighbors_of(v, u)) {
      int lvl = known_level(v, nb);
      if (lvl == 0 || lvl == VISITED) continue;
      int d_to_nb = dist[u] + 1;
      if (d_to_nb < best_dist || (d_to_nb == best_dist && nb < best_room)) {
        best_dist = d_to_nb;
        best_room = nb;
      }
    }
    for (int nb : neighbors_of(v, u)) {
      if (known_level(v, nb) != VISITED) continue;
      if (dist.count(nb)) continue;
      dist[nb] = dist[u] + 1;
      q.push(nb);
    }
  }
  if (best_room == -1) return -1;

  // Restore first step to best_room through not visited
  std::map<int, int> parent;
  std::map<int, int> d2;
  d2[cur] = 0;
  std::queue<int> q2;
  q2.push(cur);
  while (!q2.empty()) {
    int u = q2.front();
    q2.pop();
    if (u == best_room) break;
    for (int nb : neighbors_of(v, u)) {
      if (d2.count(nb)) continue;
      if (nb != best_room && known_level(v, nb) != VISITED) continue;
      d2[nb] = d2[u] + 1;
      parent[nb] = u;
      q2.push(nb);
    }
  }
  if (!d2.count(best_room)) return -1;
  int step = best_room;
  while (parent[step] != cur) step = parent[step];
  return step;
}

}  // namespace

Action AliceBot::decide(const BotView& v) {
  ensure_sized(v.current);

  // return phase
  if (returning_) {
    if (v.current == 0) return Action{ACT_STOP, 0, RES_IRON};

    int to_home = distance_to_zero_visited(v, v.current);

    if (v.food_left > to_home) {
      BestResult b = best_resource_in_room(v, v.current);
      if (b.has) return Action{ACT_COLLECT, 0, b.type};
    }

    if (return_idx_ >= (int)return_path_.size()) {
      return_path_ = shortest_path_to_zero(v, v.current);
      return_idx_ = 0;
      if (return_path_.empty()) return Action{ACT_STOP, 0, RES_IRON};
    }
    int next = return_path_[return_idx_++];
    return Action{ACT_MOVE, next, RES_IRON};
  }

  // investigate phase
  int spent = v.food_total - v.food_left;
  bool budget_exhausted = (spent >= v.food_total / 2);

  ensure_sized(v.current);
  if (!collected_here_[v.current]) {
    BestResult b = best_resource_in_room(v, v.current);
    if (b.has) {
      collected_here_[v.current] = true;
      return Action{ACT_COLLECT, 0, b.type};
    }
    collected_here_[v.current] = true;
  }

  if (budget_exhausted) {
    returning_ = true;
    return_path_ = shortest_path_to_zero(v, v.current);
    return_idx_ = 0;
    if (v.current == 0) return Action{ACT_STOP, 0, RES_IRON};
    if (return_path_.empty()) return Action{ACT_STOP, 0, RES_IRON};

    int to_home = distance_to_zero_visited(v, v.current);
    if (v.food_left > to_home) {
      BestResult b = best_resource_in_room(v, v.current);
      if (b.has) return Action{ACT_COLLECT, 0, b.type};
    }
    int next = return_path_[return_idx_++];
    return Action{ACT_MOVE, next, RES_IRON};
  }

  int next = next_explore_target(v);
  if (next == -1) {
    returning_ = true;
    return_path_ = shortest_path_to_zero(v, v.current);
    return_idx_ = 0;
    if (return_path_.empty()) return Action{ACT_STOP, 0, RES_IRON};
    int step = return_path_[return_idx_++];
    return Action{ACT_MOVE, step, RES_IRON};
  }
  return Action{ACT_MOVE, next, RES_IRON};
}
}  // namespace Bot