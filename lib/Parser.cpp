#include "Parser.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <map>
#include <sstream>
#include <vector>
namespace Parser {
namespace {

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
    out = std::stoi(tok);
    if (out < 0) return false;
    return true;
  } catch (...) {
    return false;
  }
}

bool parse_uint(const std::string& tok, size_t& out) {
  if (tok.empty()) return false;
  for (char c : tok) {
    if (!std::isdigit((unsigned char)c)) return false;
  }
  try {
    int v = std::stoi(tok);
    if (v < 0) return false;
    out = v;
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

bool split_neighbors(const std::string& s, std::vector<size_t>& out) {
  out.clear();
  if (s.empty()) return false;
  std::string cur;
  for (size_t i = 0; i <= s.size(); ++i) {
    if (i == s.size() || s[i] == ',') {
      if (cur.empty()) return false;
      size_t v;
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

struct RoomDraft {
  size_t id = -1;
  std::vector<size_t> neighbors;
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

  for (auto line = lines.begin() + 1; line != lines.end() - 1; ++line) {
    const std::string& raw = *line;
    std::vector<std::string> tok = split_ws(raw);

    if (tok.size() != 6 && !(tok.size() == 2)) {
      res.bad_line = raw;
      return res;
    }

    size_t id;
    if (!parse_uint(tok[0], id) || id > n) {
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
    for (size_t nb : r.neighbors) {
      if (nb > n || nb == id) {
        res.bad_line = raw;
        return res;
      }
    }

    if (tok.size() == 6) {
      bool any_nonzero = false;
      for (const auto& [restype, _] : BASE_VALUES) {
        int val;
        if (!parse_uint(tok[2 + restype], val) || val < 0 || val > 255) {
          res.bad_line = raw;
          return res;
        }
        r.resources[restype] = val;
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

  std::vector<Terracraft::Room> rooms;
  rooms.reserve(n + 1);
  for (int i = 0; i <= n; ++i) {
    RoomDraft& d = drafts[i];
    rooms.emplace_back(static_cast<size_t>(d.id), d.neighbors.begin(),
                       d.neighbors.end(), d.resources);
  }

  res.dungeon = std::unique_ptr<Terracraft::Dungeon>(
      new Terracraft::Dungeon(tgt, rooms.begin(), rooms.end()));
  res.M_ = m;
  res.ok = true;
  return res;
}
}  // namespace Parser