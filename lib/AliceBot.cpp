#include "AliceBot.hpp"

#include <algorithm>
#include <climits>
#include <queue>

namespace {

// Сколько еды нужно, чтобы вернуться отсюда в 0 по уже посещённым комнатам.
// Рёбра считаем неориентированными.
int distance_to_zero_visited(const BotView& v, int from) {
  if (from == 0) return 0;
  // Строим неориентированную смежность из visited.
  std::vector<std::vector<int>> adj(v.n + 1);
  for (int u = 0; u <= v.n; ++u) {
    if (v.known[u] != VISITED) continue;
    for (int x : v.neighbors[u]) {
      if (v.known[x] != VISITED) continue;
      adj[u].push_back(x);
      adj[x].push_back(u);
    }
  }
  std::vector<int> dist(v.n + 1, -1);
  std::queue<int> q;
  q.push(from);
  dist[from] = 0;
  while (!q.empty()) {
    int u = q.front();
    q.pop();
    for (int nb : adj[u]) {
      if (dist[nb] != -1) continue;
      dist[nb] = dist[u] + 1;
      if (nb == 0) return dist[nb];
      q.push(nb);
    }
  }
  return INT_MAX;
}

// Возвращает путь от from к 0 по посещённым комнатам, кратчайший.
// При нескольких кратчайших — на каждом шаге выбирается сосед с наименьшим
// номером (среди тех, что лежат на каком-либо кратчайшем пути).
// Рёбра неориентированные.
std::vector<int> shortest_path_to_zero(const BotView& v, int from) {
  std::vector<std::vector<int>> adj(v.n + 1);
  for (int u = 0; u <= v.n; ++u) {
    if (v.known[u] != VISITED) continue;
    for (int x : v.neighbors[u]) {
      if (v.known[x] != VISITED) continue;
      adj[u].push_back(x);
      adj[x].push_back(u);
    }
  }
  // BFS из 0.
  std::vector<int> dist(v.n + 1, -1);
  std::queue<int> q;
  dist[0] = 0;
  q.push(0);
  while (!q.empty()) {
    int u = q.front();
    q.pop();
    for (int nb : adj[u]) {
      if (dist[nb] != -1) continue;
      dist[nb] = dist[u] + 1;
      q.push(nb);
    }
  }
  std::vector<int> path;
  if (dist[from] == -1) return path;
  int cur = from;
  while (cur != 0) {
    int best = -1;
    for (int nb : adj[cur]) {
      if (dist[nb] != dist[cur] - 1) continue;
      if (best == -1 || nb < best) best = nb;
    }
    if (best == -1) {
      path.clear();
      return path;
    }
    path.push_back(best);
    cur = best;
  }
  return path;
}

// Самый ценный ИЗ ОСТАВШИХСЯ ресурс в комнате. -1, если ничего нет.
int best_resource_in_room(const BotView& v, int room) {
  int best = -1;
  long long best_val = -1;
  for (int k = 0; k < RES_COUNT; ++k) {
    if (v.res[room][k] <= 0) continue;
    long long val = BASE_VALUES[k];
    if ((ResourceType)k == v.target) val *= 2;
    // При равной ценности — по описанию «самой высокой ценности», порядок
    // ничей не оговорён. Берём по индексу — стабильный детерминизм.
    if (val > best_val) {
      best_val = val;
      best = k;
    }
  }
  return best;
}

// Куда идти из текущей в фазе исследования?
// (1) Среди смежных непосещённых — с наименьшим номером.
// (2) Если все смежные посещены — ближайшая непосещённая КОМНАТА из known,
//     при равенстве — с наименьшим номером. Идти к ней через посещённые.
int next_explore_target(const BotView& v) {
  int cur = v.current;
  // (1)
  int best = -1;
  for (int nb : v.neighbors[cur]) {
    if (v.known[nb] == VISITED) continue;
    if (v.known[nb] == UNKNOWN) continue;  // невидимая — не знаем о ней
    if (best == -1 || nb < best) best = nb;
  }
  if (best != -1) return best;

  // (2) Поиск ближайшей непосещённой во всём «известном» графе.
  // Расстояние — число шагов по посещённым комнатам до соседа этой
  // непосещённой. Точнее: ищем непосещённую комнату X (known>=VISIBLE),
  // достижимую так: путь от cur до соседа Y, где Y — visited, и X смежна с Y.
  // Проще: BFS по visited, и для каждой visited смотрим, есть ли непосещённые
  // соседи.
  std::vector<int> dist(v.n + 1, -1);
  std::queue<int> q;
  dist[cur] = 0;
  q.push(cur);
  int best_dist = INT_MAX, best_room = -1;
  while (!q.empty()) {
    int u = q.front();
    q.pop();
    // Смотрим непосещённых соседей u — они кандидаты.
    for (int nb : v.neighbors[u]) {
      if (v.known[nb] == VISITED || v.known[nb] == UNKNOWN) continue;
      int d_to_nb = dist[u] + 1;
      if (d_to_nb < best_dist || (d_to_nb == best_dist && nb < best_room)) {
        best_dist = d_to_nb;
        best_room = nb;
      }
    }
    // Расширяем BFS только по visited.
    for (int nb : v.neighbors[u]) {
      if (v.known[nb] != VISITED) continue;
      if (dist[nb] != -1) continue;
      dist[nb] = dist[u] + 1;
      q.push(nb);
    }
  }
  if (best_room == -1) return -1;
  // Идти к нему — следующий шаг по кратчайшему пути от cur.
  // Снова BFS: восстановим, какой сосед cur ведёт к best_room (через
  // посещённые). Делаем BFS, помня предков.
  std::vector<int> parent(v.n + 1, -1);
  std::vector<int> d2(v.n + 1, -1);
  d2[cur] = 0;
  std::queue<int> q2;
  q2.push(cur);
  while (!q2.empty()) {
    int u = q2.front();
    q2.pop();
    if (u == best_room) break;
    for (int nb : v.neighbors[u]) {
      if (d2[nb] != -1) continue;
      // Идём по visited, либо это и есть цель.
      if (nb != best_room && v.known[nb] != VISITED) continue;
      d2[nb] = d2[u] + 1;
      parent[nb] = u;
      q2.push(nb);
    }
  }
  if (d2[best_room] == -1) return -1;
  int step = best_room;
  while (parent[step] != cur) step = parent[step];
  return step;
}

}  // namespace

Action AliceBot::decide(const BotView& v) {
  ensure_sized(v.n);

  // === Фаза возврата ===
  if (returning_) {
    // Если стоим в 0 — стоп.
    if (v.current == 0) return Action{ACT_STOP, 0, RES_IRON};

    // Сколько шагов до 0?
    int to_home = distance_to_zero_visited(v, v.current);

    // Сколько еды нужно МИНИМУМ, чтобы дойти? Ровно `to_home` (шаги).
    // Если food_left > to_home — есть «лишняя» еда, можно собирать.
    if (v.food_left > to_home) {
      // Собираем самый ценный ресурс в текущей комнате (если есть).
      int k = best_resource_in_room(v, v.current);
      if (k >= 0) {
        return Action{ACT_COLLECT, 0, (ResourceType)k};
      }
      // Здесь ничего нет — шагаем дальше.
    }

    // Идём по заранее посчитанному пути.
    if (return_idx_ >= (int)return_path_.size()) {
      // Путь устарел (например, мы в нём не дальше шли). Пересчитаем.
      return_path_ = shortest_path_to_zero(v, v.current);
      return_idx_ = 0;
      if (return_path_.empty()) return Action{ACT_STOP, 0, RES_IRON};
    }
    int next = return_path_[return_idx_++];
    return Action{ACT_MOVE, next, RES_IRON};
  }

  // === Фаза исследования ===
  // Бюджет фазы — M/2. Условие выхода: потрачено >= M/2.
  int spent = v.food_total - v.food_left;
  bool budget_exhausted = (spent >= v.food_total / 2);

  // Сначала проверим: если в ТЕКУЩЕЙ комнате есть несобранный топ-ресурс
  // и мы в неё только что вошли (или это самое первое решение),
  // нужно его собрать. Эвристика: «в новой комнате» = первый сбор в этой
  // комнате ещё не делался, и ресурсы есть.
  // Признак «первый сбор не делался» определяется так: ни один ресурс в
  // комнате не равен 0, ИЛИ некоторые ресурсы изначально были 0 — это уже
  // плохо отличимо. Но симулятор разрешает первый сбор бесплатно ровно
  // один раз. Поэтому правило простое: пока в комнате есть лучший ресурс
  // и за этот заход мы здесь ещё не собирали — собираем.
  //
  // Способ отличить «уже собирал тут» от «не собирал»: симулятор после
  // нашего ACT_COLLECT обнулит соответствующий res, но другие останутся
  // ненулевыми. То есть из BotView мы не можем чисто отличить. Решаем
  // проще: ведём собственный set посещённых-с-собранным. Но мы — функция
  // decide. Сделаем член класса.
  //
  // ---> вместо мутного эвристика добавим поле collected_here_.
  if (!collected_here_[v.current]) {
    int k = best_resource_in_room(v, v.current);
    if (k >= 0) {
      collected_here_[v.current] = true;
      return Action{ACT_COLLECT, 0, (ResourceType)k};
    }
    // Если ресурсов нет вообще — тоже помечаем, чтобы не зацикливаться.
    collected_here_[v.current] = true;
  }

  // Бюджет на исследование закончился — переходим к возврату.
  if (budget_exhausted) {
    returning_ = true;
    return_path_ = shortest_path_to_zero(v, v.current);
    return_idx_ = 0;
    if (v.current == 0) return Action{ACT_STOP, 0, RES_IRON};
    if (return_path_.empty()) return Action{ACT_STOP, 0, RES_IRON};

    // Сразу первый ход возврата (может быть и сбор, если еды с запасом).
    int to_home = distance_to_zero_visited(v, v.current);
    if (v.food_left > to_home) {
      int k = best_resource_in_room(v, v.current);
      if (k >= 0) return Action{ACT_COLLECT, 0, (ResourceType)k};
    }
    int next = return_path_[return_idx_++];
    return Action{ACT_MOVE, next, RES_IRON};
  }

  // Ещё есть бюджет — выбираем следующую комнату для исследования.
  int next = next_explore_target(v);
  if (next == -1) {
    // Идти некуда — начинаем возвращаться.
    returning_ = true;
    return_path_ = shortest_path_to_zero(v, v.current);
    return_idx_ = 0;
    if (return_path_.empty()) return Action{ACT_STOP, 0, RES_IRON};
    int step = return_path_[return_idx_++];
    return Action{ACT_MOVE, step, RES_IRON};
  }
  return Action{ACT_MOVE, next, RES_IRON};
}