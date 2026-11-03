// Минимальный ustar: чтение и запись. Формат пакета .solpkg - обычный несжатый tar,
// его можно открыть любым tar. Допустимы только файлы, каталоги и симлинки.
#pragma once
#include <cstring>
#include <string>
#include <vector>
#include "control.hpp"

namespace sp {

struct TarEntry {
    std::string path;
    char type = '0';          // '0' файл, '5' каталог, '2' симлинк
    unsigned mode = 0644;
    std::string data;         // содержимое (для файлов)
    std::string link;         // цель (для симлинков)
};

namespace detail {
inline size_t cstrlen(const char* s, size_t max) { size_t n = 0; while (n < max && s[n]) n++; return n; }

inline void put_octal(char* dst, size_t n, unsigned long long v) {   // n включает завершающий NUL
    for (size_t i = n - 1; i-- > 0;) { dst[i] = char('0' + (v & 7)); v >>= 3; }
    dst[n - 1] = '\0';
}
inline unsigned long long get_octal(const char* s, size_t n) {
    size_t i = 0; while (i < n && s[i] == ' ') i++;
    unsigned long long v = 0;
    for (; i < n && s[i] >= '0' && s[i] <= '7'; i++) v = (v << 3) | (unsigned long long)(s[i] - '0');
    return v;
}
} // namespace detail

inline std::string tar_write(const std::vector<TarEntry>& entries) {
    using namespace detail;
    std::string out;
    for (const auto& e : entries) {
        char h[512]; std::memset(h, 0, sizeof h);
        std::string path = e.path;
        if (e.type == '5' && (path.empty() || path.back() != '/')) path += '/';
        std::string name = path, prefix;
        if (name.size() > 100) {
            bool ok = false;
            for (size_t i = 0; i < path.size(); i++) {
                size_t rest = path.size() - i - 1;
                if (path[i] == '/' && i <= 155 && rest > 0 && rest <= 100) {
                    prefix = path.substr(0, i); name = path.substr(i + 1); ok = true; break;
                }
            }
            if (!ok) throw Error("path too long for ustar: " + e.path);
        }
        if (e.link.size() > 100) throw Error("symlink target too long: " + e.path);
        std::memcpy(h, name.data(), name.size());
        put_octal(h + 100, 8, e.mode & 07777);
        put_octal(h + 108, 8, 0);                    // uid
        put_octal(h + 116, 8, 0);                    // gid
        put_octal(h + 124, 12, e.type == '0' ? e.data.size() : 0);
        put_octal(h + 136, 12, 0);                   // mtime = 0: воспроизводимые сборки
        std::memset(h + 148, ' ', 8);
        h[156] = e.type;
        std::memcpy(h + 157, e.link.data(), e.link.size());
        std::memcpy(h + 257, "ustar", 6);
        std::memcpy(h + 263, "00", 2);
        std::memcpy(h + 265, "root", 5);
        std::memcpy(h + 297, "root", 5);
        put_octal(h + 329, 8, 0); put_octal(h + 337, 8, 0);
        std::memcpy(h + 345, prefix.data(), prefix.size());
        unsigned sum = 0; for (int i = 0; i < 512; i++) sum += (unsigned char)h[i];
        put_octal(h + 148, 7, sum); h[155] = ' ';
        out.append(h, 512);
        if (e.type == '0') {
            out += e.data;
            size_t pad = (512 - e.data.size() % 512) % 512;
            out.append(pad, '\0');
        }
    }
    out.append(1024, '\0');
    return out;
}

inline std::vector<TarEntry> tar_read(const std::string& buf) {
    using namespace detail;
    std::vector<TarEntry> out; size_t pos = 0;
    while (pos + 512 <= buf.size()) {
        const char* h = buf.data() + pos;
        bool zero = true; for (int i = 0; i < 512; i++) if (h[i]) { zero = false; break; }
        if (zero) break;
        if (std::memcmp(h + 257, "ustar", 5) != 0) throw Error("not a ustar archive");
        unsigned long long stored = get_octal(h + 148, 8);
        unsigned sum = 0;
        for (int i = 0; i < 512; i++) sum += (i >= 148 && i < 156) ? (unsigned)' ' : (unsigned char)h[i];
        if (sum != stored) throw Error("tar header checksum mismatch");

        TarEntry e;
        std::string name(h, cstrlen(h, 100)), prefix(h + 345, cstrlen(h + 345, 155));
        e.path = prefix.empty() ? name : prefix + "/" + name;
        e.type = h[156] == '\0' ? '0' : h[156];
        e.mode = (unsigned)get_octal(h + 100, 8);
        unsigned long long size = get_octal(h + 124, 12);
        e.link.assign(h + 157, cstrlen(h + 157, 100));
        if (e.type != '0' && e.type != '5' && e.type != '2')
            throw Error(std::string("unsupported tar entry type '") + e.type + "' for " + e.path);
        pos += 512;
        if (e.type == '0') {
            if (pos + size > buf.size()) throw Error("truncated archive");
            e.data.assign(buf, pos, (size_t)size);
            pos += (size_t)((size + 511) / 512 * 512);
        }
        out.push_back(std::move(e));
    }
    return out;
}

} // namespace sp
