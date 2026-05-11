#include "Dungeon.hpp"

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace {

// Убирает \r в конце (на случай Windows-переносов) и пробелы по краям.
std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

// Парсит целое из строки СТРОГО: только цифры, без знака, без хвоста.
// Возвращает false при любом мусоре.
bool parse_uint(const std::string& tok, int& out) {
    if (tok.empty()) return false;
    for (char c : tok) {
        if (!std::isdigit((unsigned char)c)) return false;
    }
    try {
        long v = std::stol(tok);
        if (v < 0 || v > 1000000) return false; // диапазон проверим выше
        out = (int)v;
        return true;
    } catch (...) {
        return false;
    }
}

// Разбивает строку по пробелам на токены.
std::vector<std::string> split_ws(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string t;
    while (iss >> t) out.push_back(t);
    return out;
}

// Разбивает "1,2,3" по запятым. Возвращает false, если есть
// пустые элементы или нечисловые символы.
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
    if (s == "iron") { out = RES_IRON; return true; }
    if (s == "gold") { out = RES_GOLD; return true; }
    if (s == "gems") { out = RES_GEMS; return true; }
    if (s == "exp")  { out = RES_EXP;  return true; }
    return false;
}

} // namespace

ParseResult load_dungeon(const std::string& path, Dungeon& out) {
    ParseResult res;
    res.ok = false;

    std::ifstream fin(path);
    if (!fin.is_open()) {
        res.bad_line = "cannot open file: " + path;
        return res;
    }

    // Читаем все строки сразу — так удобнее проверять структуру и
    // возвращать оригинальную строку при ошибке.
    std::vector<std::string> lines;
    {
        std::string line;
        while (std::getline(fin, line)) {
            // Уберём \r, но сохраним строку как есть для вывода.
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines.push_back(line);
        }
    }

    if (lines.empty()) {
        res.bad_line = "";
        return res;
    }

    // --- Строка 1: N ---
    std::string first = trim(lines[0]);
    int n;
    if (!parse_uint(first, n) || n < 1 || n > 255) {
        res.bad_line = lines[0];
        return res;
    }

    // Ожидаем дальше N+1 строк описания комнат и 1 строку с едой+целью.
    // Итого нужно как минимум 1 + (N+1) + 1 = N+3 строк.
    if ((int)lines.size() < n + 3) {
        // Не хватает строк — сообщим про последнюю прочитанную.
        res.bad_line = lines.back();
        return res;
    }

    out.n = n;
    out.rooms.assign(n + 1, Room{});
    std::vector<bool> seen(n + 1, false);

    // --- Строки 2..N+2: комнаты ---
    for (int i = 0; i <= n; ++i) {
        const std::string& raw = lines[1 + i];
        std::vector<std::string> tok = split_ws(raw);

        // Формат комнаты:
        //   <id> <neighbors> <iron> <gold> <gems> <exp>     — обычная комната
        //   <id> <neighbors>                                — стартовая (0)
        //
        // По примеру стартовая может идти БЕЗ ресурсов. Поддержим оба
        // варианта (6 токенов для всех, либо 2 токена для комнаты 0).

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
            // Дубль номера комнаты.
            res.bad_line = raw;
            return res;
        }
        seen[id] = true;

        Room& r = out.rooms[id];
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

        if (tok.size() == 6) {
            // У стартовой не должно быть ресурсов.
            int vals[4];
            for (int k = 0; k < 4; ++k) {
                if (!parse_uint(tok[2 + k], vals[k]) || vals[k] < 0 || vals[k] > 255) {
                    res.bad_line = raw;
                    return res;
                }
            }
            if (id == 0 && (vals[0] || vals[1] || vals[2] || vals[3])) {
                res.bad_line = raw;
                return res;
            }
            for (int k = 0; k < 4; ++k) r.resources[k] = vals[k];
        } else {
            // tok.size() == 2: допустимо только для стартовой.
            if (id != 0) {
                res.bad_line = raw;
                return res;
            }
            for (int k = 0; k < 4; ++k) r.resources[k] = 0;
        }
    }

    // Проверяем, что все номера 0..N встретились.
    for (int i = 0; i <= n; ++i) {
        if (!seen[i]) {
            res.bad_line = lines[1 + i < (int)lines.size() ? 1 + i : (int)lines.size() - 1];
            return res;
        }
    }

    // Симметрию рёбер НЕ проверяем: в примере из задания у комнаты 4
    // в списке соседей нет 3, хотя у комнаты 3 есть 4. Видимо, проходы
    // могут быть однонаправленными (или это допустимая «несогласованность»
    // на уровне формата). Бот будет ходить только по тем рёбрам,
    // которые объявлены у текущей комнаты.

    // --- Последняя строка: M и целевой ресурс ---
    const std::string& last = lines[1 + (n + 1)]; // = lines[n+2]
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
    out.food = m;
    out.target = tgt;

    res.ok = true;
    return res;
}