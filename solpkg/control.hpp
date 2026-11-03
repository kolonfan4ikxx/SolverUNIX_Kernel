// Разбор control-файлов (формат как у Debian), сравнение версий, зависимости.
#pragma once
#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sp {

struct Error : std::runtime_error { using std::runtime_error::runtime_error; };

// ---- stanza: упорядоченный набор "Ключ: значение" ----
struct Stanza {
    std::vector<std::pair<std::string, std::string>> f;

    const std::string* get(const std::string& k) const {
        for (auto& p : f) if (p.first == k) return &p.second;
        return nullptr;
    }
    std::string value(const std::string& k, const std::string& def = "") const {
        auto p = get(k); return p ? *p : def;
    }
    void set(const std::string& k, const std::string& v) {
        for (auto& p : f) if (p.first == k) { p.second = v; return; }
        f.emplace_back(k, v);
    }
    std::string str() const {
        std::string s;
        for (auto& p : f) {
            s += p.first + ": ";
            for (char c : p.second) { s += c; if (c == '\n') s += ' '; }
            s += '\n';
        }
        return s;
    }
};

inline std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) a++;
    while (b > a && std::isspace((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
}

inline std::vector<Stanza> parse_stanzas(const std::string& text) {
    std::vector<Stanza> out; Stanza cur;
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t nl = text.find('\n', pos);
        std::string line = text.substr(pos, nl == std::string::npos ? std::string::npos : nl - pos);
        pos = (nl == std::string::npos) ? text.size() + 1 : nl + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (trim(line).empty()) { if (!cur.f.empty()) { out.push_back(cur); cur = Stanza(); } continue; }
        if (line[0] == '#') continue;
        if (line[0] == ' ' || line[0] == '\t') {
            if (cur.f.empty()) throw Error("control: continuation line without a field");
            cur.f.back().second += "\n" + trim(line);
            continue;
        }
        size_t c = line.find(':');
        if (c == std::string::npos || c == 0) throw Error("control: malformed line '" + line + "'");
        cur.f.emplace_back(line.substr(0, c), trim(line.substr(c + 1)));
    }
    if (!cur.f.empty()) out.push_back(cur);
    return out;
}

// ---- сравнение версий (алгоритм dpkg без epoch/revision-разбиения) ----
inline int vorder(char c) {
    if (std::isdigit((unsigned char)c)) return 0;
    if (std::isalpha((unsigned char)c)) return c;
    if (c == '~') return -1;
    if (c) return (unsigned char)c + 256;
    return 0;
}
inline int vercmp(const std::string& A, const std::string& B) {
    const char* a = A.c_str(); const char* b = B.c_str();
    while (*a || *b) {
        int first_diff = 0;
        while ((*a && !std::isdigit((unsigned char)*a)) || (*b && !std::isdigit((unsigned char)*b))) {
            int ac = vorder(*a), bc = vorder(*b);
            if (ac != bc) return ac < bc ? -1 : 1;
            a++; b++;
        }
        while (*a == '0') a++;
        while (*b == '0') b++;
        while (std::isdigit((unsigned char)*a) && std::isdigit((unsigned char)*b)) {
            if (!first_diff) first_diff = *a - *b;
            a++; b++;
        }
        if (std::isdigit((unsigned char)*a)) return 1;
        if (std::isdigit((unsigned char)*b)) return -1;
        if (first_diff) return first_diff < 0 ? -1 : 1;
    }
    return 0;
}

// ---- зависимости: "a, b (>= 1.0) | c" ----
struct Dep { std::string name, op, ver; };
using Alt = std::vector<Dep>;

inline std::vector<Alt> parse_depends(const std::string& s) {
    std::vector<Alt> out;
    size_t i = 0;
    while (i <= s.size()) {
        size_t j = s.find(',', i); if (j == std::string::npos) j = s.size();
        std::string item = trim(s.substr(i, j - i)); i = j + 1;
        if (item.empty()) continue;
        Alt alt; size_t k = 0;
        while (k <= item.size()) {
            size_t m = item.find('|', k); if (m == std::string::npos) m = item.size();
            std::string d = trim(item.substr(k, m - k)); k = m + 1;
            if (d.empty()) continue;
            Dep dep; size_t p = d.find('(');
            if (p == std::string::npos) dep.name = trim(d);
            else {
                size_t q = d.find(')', p);
                if (q == std::string::npos) throw Error("bad dependency: " + d);
                dep.name = trim(d.substr(0, p));
                std::string v = trim(d.substr(p + 1, q - p - 1));
                size_t o = 0; while (o < v.size() && std::strchr("<>=", v[o])) o++;
                dep.op = v.substr(0, o); dep.ver = trim(v.substr(o));
                static const char* ok[] = {">=", "<=", ">>", "<<", "=", ">", "<"};
                bool good = false; for (auto x : ok) if (dep.op == x) good = true;
                if (!good || dep.ver.empty()) throw Error("bad version constraint: " + d);
            }
            alt.push_back(dep);
        }
        if (!alt.empty()) out.push_back(alt);
    }
    return out;
}

inline bool satisfies(const std::string& have, const Dep& d) {
    if (d.op.empty()) return true;
    int c = vercmp(have, d.ver);
    if (d.op == ">=") return c >= 0;
    if (d.op == "<=") return c <= 0;
    if (d.op == ">>" || d.op == ">") return c > 0;
    if (d.op == "<<" || d.op == "<") return c < 0;
    return c == 0;
}

} // namespace sp
