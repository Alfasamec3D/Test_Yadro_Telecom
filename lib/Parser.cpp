#include "Parser.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>

namespace {

const std::array<ResourceType, 4>& resource_order() {
  static const std::array<ResourceType, 4> order{RES_IRON, RES_GOLD, RES_GEMS,
                                                 RES_EXP};
  return order;
}

std::string trim(const std::string& s) {
  size_t a = 0, b = s.size();
  while (a < b && std::isspace((unsigned char)s[a])) ++a;
  while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
  return s.substr(a, b - a);
}

bool parse_uint(const std::string& tok, int& out) {
  if (tok.empty()) return false;
  for (char c : tok) {
    if (!std::isdigit((unsigned char)c)) return false;
  }
  try {
    long v = std::stol(tok);
    if (v < 0 || v > 1000000) return false;
    out = (int)v;
    return true;
  } catch (...) {
    return false;
  }
}

std::vector<std::string> split_ws(const std::string& s) {
  std::vector<std::string> out;
  std::istringstream iss(s);
  std::string t;
  while (iss >> t) out.push_back(t);
  return out;
}

bool split_neighbors(const std::string& s, std::vector<int>& out) {
  out.clear();
  if (s.empty()) return false;
  std::string cur;
  for (size_t i = 0; i <= s.size(); ++i) {
    if (i == s.size() || s[i] == ',') {
      if (cur.empty()) return false;
      int v;
      if (!parse_uint(cur, v)) return false;
      out.push_back(v);
      cur.clear();
    } else {
      cur.push_back(s[i]);
    }
  }
  return true;
}

bool parse_target(const std::string& s, ResourceType& out) {
  for (const auto& [type, name] : RES_NAMES) {
    if (s == name) {
      out = type;
      return true;
    }
  }
  return false;
}

// Промежуточный POD для первой фазы — у Room const-поля, мы не можем
// держать в std::vector<Room> «недозаполненные» комнаты. Сначала всё
// читаем сюда, потом конструируем настоящие Room.
struct RoomDraft {
  int id = -1;
  std::vector<int> neighbors;
  std::map<ResourceType, int> resources;
};

}  // namespace

ParseResult load_dungeon(const std::string& path) {
  ParseResult res;
  res.ok = false;

  std::ifstream fin(path);
  if (!fin.is_open()) {
    res.bad_line = "cannot open file: " + path;
    return res;
  }

  std::vector<std::string> lines;
  {
    std::string line;
    while (std::getline(fin, line)) {
      if (!line.empty() && line.back() == '\r') line.pop_back();
      lines.push_back(line);
    }
  }
  if (lines.empty()) {
    res.bad_line = "";
    return res;
  }

  std::string first = trim(lines[0]);
  int n;
  if (!parse_uint(first, n) || n < 1 || n > 255) {
    res.bad_line = lines[0];
    return res;
  }

  if ((int)lines.size() < n + 3) {
    res.bad_line = lines.back();
    return res;
  }

  std::vector<RoomDraft> drafts(n + 1);
  std::vector<bool> seen(n + 1, false);
  const auto& order = resource_order();

  for (int i = 0; i <= n; ++i) {
    const std::string& raw = lines[1 + i];
    std::vector<std::string> tok = split_ws(raw);

    if (tok.size() != 6 && !(tok.size() == 2)) {
      res.bad_line = raw;
      return res;
    }

    int id;
    if (!parse_uint(tok[0], id) || id < 0 || id > n) {
      res.bad_line = raw;
      return res;
    }
    if (seen[id]) {
      res.bad_line = raw;
      return res;
    }
    seen[id] = true;

    RoomDraft& r = drafts[id];
    r.id = id;
    if (!split_neighbors(tok[1], r.neighbors)) {
      res.bad_line = raw;
      return res;
    }
    for (int nb : r.neighbors) {
      if (nb < 0 || nb > n || nb == id) {
        res.bad_line = raw;
        return res;
      }
    }

    for (ResourceType rt : order) r.resources[rt] = 0;

    if (tok.size() == 6) {
      bool any_nonzero = false;
      for (size_t k = 0; k < order.size(); ++k) {
        int val;
        if (!parse_uint(tok[2 + k], val) || val < 0 || val > 255) {
          res.bad_line = raw;
          return res;
        }
        r.resources[order[k]] = val;
        if (val) any_nonzero = true;
      }
      if (id == 0 && any_nonzero) {
        res.bad_line = raw;
        return res;
      }
    } else {
      if (id != 0) {
        res.bad_line = raw;
        return res;
      }
    }
  }

  for (int i = 0; i <= n; ++i) {
    if (!seen[i]) {
      res.bad_line =
          lines[1 + i < (int)lines.size() ? 1 + i : (int)lines.size() - 1];
      return res;
    }
  }

  const std::string& last = lines[1 + (n + 1)];
  std::vector<std::string> ltok = split_ws(last);
  if (ltok.size() != 2) {
    res.bad_line = last;
    return res;
  }
  int m;
  if (!parse_uint(ltok[0], m) || m < 2 || m > 255) {
    res.bad_line = last;
    return res;
  }
  ResourceType tgt;
  if (!parse_target(ltok[1], tgt)) {
    res.bad_line = last;
    return res;
  }

  // Вторая фаза — конструируем настоящие Room из черновиков.
  std::vector<Room> rooms;
  rooms.reserve(n + 1);
  for (int i = 0; i <= n; ++i) {
    RoomDraft& d = drafts[i];
    rooms.emplace_back(static_cast<size_t>(d.id), d.neighbors.begin(),
                       d.neighbors.end(), d.resources);
  }

  res.dungeon =
      std::unique_ptr<Dungeon>(new Dungeon(tgt, rooms.begin(), rooms.end()));
  res.M_ = m;
  res.ok = true;
  return res;
}