// solpkg - пакетный менеджер SolverUNIX.
//
// Пакет .solpkg  = несжатый ustar-архив: первым идёт SOLPKG/control, далее файлы
//                  относительно корня. Скриптов обслуживания нет (пакеты декларативные).
// Репозиторий    = каталог с *.solpkg и индексом Packages (Filename, Size, SHA256).
// База           = <root>/var/lib/solpkg/{status,available,info/<pkg>.list}
//
// Сейчас собирается как хостовая утилита (g++ -std=c++17); цель - перенос на SolverUNIX
// поверх solibc. Всё, что трогает ФС, идёт через <filesystem>.
#include <sys/stat.h>
#include <unistd.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#include "control.hpp"
#include "sha256.hpp"
#include "tar.hpp"

namespace fs = std::filesystem;
using namespace sp;

static const char* VERSION   = "0.1.0";
static const char* HOST_ARCH = "x86_64";

// ===================================================================== контекст
struct Ctx {
    fs::path root = "/";
    bool root_explicit = false;
    std::vector<fs::path> repos;
    bool yes = false, force = false, installed_only = false;
    std::string out;

    fs::path dbdir() const { return root / "var/lib/solpkg"; }
    fs::path status_path() const { return dbdir() / "status"; }
    fs::path avail_path() const { return dbdir() / "available"; }
    fs::path info_dir() const { return dbdir() / "info"; }
    fs::path list_path(const std::string& pkg) const { return info_dir() / (pkg + ".list"); }
};

// ===================================================================== утилиты
static std::string read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    if (!f) throw Error("cannot read " + p.string());
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}

static void write_file(const fs::path& p, const std::string& data) {
    fs::create_directories(p.parent_path());
    fs::path tmp = p; tmp += ".solpkg-tmp";
    {
        std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
        if (!f) throw Error("cannot write " + tmp.string());
        f.write(data.data(), (std::streamsize)data.size());
        f.flush();
        if (!f) throw Error("write failed: " + tmp.string());
    }
    fs::rename(tmp, p);
}

static bool valid_name(const std::string& n) {
    if (n.size() < 2) return false;
    if (!std::isalnum((unsigned char)n[0]) || std::isupper((unsigned char)n[0])) return false;
    for (char c : n)
        if (!(std::islower((unsigned char)c) || std::isdigit((unsigned char)c) || c == '+' || c == '-' || c == '.'))
            return false;
    return true;
}

// Нормализация пути из архива: относительный, без "..", без "." внутри. "" = корень (пропустить).
static std::string clean_rel(std::string p) {
    while (p.rfind("./", 0) == 0) p.erase(0, 2);
    while (!p.empty() && p.back() == '/') p.pop_back();
    if (p.empty() || p == ".") return "";
    if (p[0] == '/') throw Error("absolute path in package: " + p);
    if (p.find('\0') != std::string::npos || p.find('\n') != std::string::npos)
        throw Error("illegal character in path");
    size_t i = 0;
    while (i <= p.size()) {
        size_t j = p.find('/', i); if (j == std::string::npos) j = p.size();
        std::string comp = p.substr(i, j - i); i = j + 1;
        if (comp.empty() || comp == "." || comp == "..") throw Error("unsafe path in package: " + p);
    }
    return p;
}

// Ни один каталог на пути (внутри root) не должен быть симлинком: защита от выхода из корня.
static void check_no_symlink_parents(const fs::path& root, const std::string& rel) {
    fs::path cur = root;
    for (auto& comp : fs::path(rel).parent_path()) {
        cur /= comp;
        std::error_code ec;
        auto st = fs::symlink_status(cur, ec);
        if (!ec && fs::is_symlink(st)) throw Error("refusing to write through symlink: " + cur.string());
    }
}

static fs::perms to_perms(unsigned mode) { return static_cast<fs::perms>(mode & 07777); }
static unsigned from_perms(fs::perms p) { return static_cast<unsigned>(p) & 07777; }

// ===================================================================== база данных
struct ListEntry { char kind; std::string sha, path; };   // D / F / L

static std::vector<ListEntry> read_list(const Ctx& c, const std::string& pkg) {
    std::vector<ListEntry> out;
    fs::path p = c.list_path(pkg);
    if (!fs::exists(p)) return out;
    std::istringstream in(read_file(p));
    std::string line;
    while (std::getline(in, line)) {
        if (line.size() < 3) continue;
        ListEntry e; e.kind = line[0];
        if (e.kind == 'F') {
            size_t sp2 = line.find(' ', 2);
            if (sp2 == std::string::npos) continue;
            e.sha = line.substr(2, sp2 - 2); e.path = line.substr(sp2 + 1);
        } else e.path = line.substr(2);
        out.push_back(e);
    }
    return out;
}

struct Db {
    const Ctx& c;
    std::map<std::string, Stanza> installed;

    explicit Db(const Ctx& ctx) : c(ctx) { load(); }

    void load() {
        installed.clear();
        if (!fs::exists(c.status_path())) return;
        for (auto& s : parse_stanzas(read_file(c.status_path()))) installed[s.value("Package")] = s;
    }
    void save() const {
        std::string out;
        for (auto& kv : installed) out += kv.second.str() + "\n";
        write_file(c.status_path(), out);
    }
};

struct Avail { Stanza s; std::string name, version, repo; };

static std::vector<Avail> load_available(const Ctx& c) {
    std::vector<Avail> out;
    if (!fs::exists(c.avail_path())) throw Error("no package lists; run 'solpkg update' first");
    for (auto& s : parse_stanzas(read_file(c.avail_path())))
        out.push_back({s, s.value("Package"), s.value("Version"), s.value("Repo")});
    return out;
}

static const Avail* best(const std::vector<Avail>& av, const Dep& d) {
    const Avail* b = nullptr;
    for (auto& a : av) {
        if (a.name != d.name) continue;
        std::string ar = a.s.value("Architecture", "all");
        if (ar != "all" && ar != HOST_ARCH) continue;
        if (!satisfies(a.version, d)) continue;
        if (!b || vercmp(a.version, b->version) > 0) b = &a;
    }
    return b;
}

static std::map<std::string, std::string> load_owners(const Ctx& c, const Db& db, const std::string& skip) {
    std::map<std::string, std::string> o;
    for (auto& kv : db.installed) {
        if (kv.first == skip) continue;
        for (auto& e : read_list(c, kv.first)) if (e.kind != 'D') o[e.path] = kv.first;
    }
    return o;
}

// ===================================================================== планирование
static void plan_visit(const Dep& want, const Db& db, const std::vector<Avail>& av,
                       std::vector<const Avail*>& order, std::set<std::string>& visiting,
                       std::set<std::string>& done, bool top)
{
    auto inst = db.installed.find(want.name);
    bool have = inst != db.installed.end();
    if (have && !top && satisfies(inst->second.value("Version"), want)) return;

    const Avail* a = best(av, want);
    if (!a) {
        if (have && top) return;
        throw Error("package '" + want.name + (want.op.empty() ? "" : " (" + want.op + " " + want.ver + ")") +
                    "' is not available");
    }
    if (have && top && vercmp(a->version, inst->second.value("Version")) <= 0) {
        std::cout << want.name << " is already the newest version (" << inst->second.value("Version") << ")\n";
        return;
    }
    if (done.count(a->name) || visiting.count(a->name)) return;   // повтор или цикл
    visiting.insert(a->name);

    for (auto& alt : parse_depends(a->s.value("Depends"))) {
        bool ok = false;
        for (auto& d : alt) {
            auto i2 = db.installed.find(d.name);
            if (i2 != db.installed.end() && satisfies(i2->second.value("Version"), d)) { ok = true; break; }
            for (auto* p : order) if (p->name == d.name && satisfies(p->version, d)) ok = true;
            if (visiting.count(d.name)) ok = true;                // цикл зависимостей - не зацикливаемся
            if (ok) break;
        }
        if (ok) continue;
        const Dep* pick = nullptr;
        for (auto& d : alt) if (best(av, d)) { pick = &d; break; }
        if (!pick) {
            std::string names; for (auto& d : alt) names += (names.empty() ? "" : " | ") + d.name;
            throw Error("unmet dependency of " + a->name + ": " + names);
        }
        plan_visit(*pick, db, av, order, visiting, done, false);
    }
    visiting.erase(a->name);
    done.insert(a->name);
    order.push_back(a);
}

static bool confirm(const Ctx& c) {
    if (c.yes) return true;
    if (!isatty(0)) throw Error("not a terminal; re-run with -y to proceed");
    std::cout << "Do you want to continue? [Y/n] " << std::flush;
    std::string a; std::getline(std::cin, a);
    return a.empty() || a[0] == 'y' || a[0] == 'Y';
}

// ===================================================================== установка
static void install_one(const Ctx& c, Db& db, const Avail& a) {
    std::string filename = a.s.value("Filename");
    if (filename.empty()) throw Error("index has no Filename for " + a.name);
    std::string want = a.s.value("SHA256");
    if (want.empty()) throw Error("index has no SHA256 for " + a.name + " (refusing unverified package)");

    std::string blob = read_file(fs::path(a.repo) / filename);
    if (sha256_hex(blob) != want) throw Error("checksum mismatch for " + a.name + " (" + filename + ")");

    auto entries = tar_read(blob);
    if (entries.empty() || entries[0].path != "SOLPKG/control")
        throw Error("malformed package " + a.name + ": SOLPKG/control must be the first entry");
    auto cs = parse_stanzas(entries[0].data);
    if (cs.size() != 1) throw Error("malformed control in " + a.name);
    Stanza ctl = cs[0];
    if (ctl.value("Package") != a.name || ctl.value("Version") != a.version)
        throw Error("package contents do not match the index for " + a.name);
    std::string arch = ctl.value("Architecture", "all");
    if (arch != "all" && arch != HOST_ARCH) throw Error(a.name + ": wrong architecture " + arch);

    struct Item { std::string rel; const TarEntry* e; };
    std::vector<Item> items; std::set<std::string> seen;
    for (size_t i = 1; i < entries.size(); i++) {
        const TarEntry& e = entries[i];
        if (e.path == "SOLPKG" || e.path.rfind("SOLPKG/", 0) == 0) continue;   // зарезервировано
        std::string rel = clean_rel(e.path);
        if (rel.empty()) continue;
        if (!seen.insert(rel).second) throw Error("duplicate path in package: " + rel);
        items.push_back({rel, &e});
    }

    auto owners = load_owners(c, db, a.name);
    for (auto& it : items)
        if (it.e->type != '5') {
            auto o = owners.find(it.rel);
            if (o != owners.end()) throw Error("'/" + it.rel + "' is also in package " + o->second);
        }

    std::string list;
    for (auto& it : items) {
        const TarEntry& e = *it.e;
        fs::path dest = c.root / it.rel;
        check_no_symlink_parents(c.root, it.rel);
        std::error_code ec;
        auto st = fs::symlink_status(dest, ec);
        if (e.type == '5') {
            if (fs::exists(st) && !fs::is_directory(st)) throw Error("not a directory: " + dest.string());
            fs::create_directories(dest);
            fs::permissions(dest, to_perms(e.mode), fs::perm_options::replace);
            list += "D " + it.rel + "\n";
        } else if (e.type == '2') {
            if (fs::exists(st) && fs::is_directory(st) && !fs::is_symlink(st)) throw Error("is a directory: " + dest.string());
            fs::create_directories(dest.parent_path());
            fs::remove(dest, ec);
            fs::create_symlink(e.link, dest);
            list += "L " + it.rel + "\n";
        } else {
            if (fs::exists(st) && fs::is_directory(st) && !fs::is_symlink(st)) throw Error("is a directory: " + dest.string());
            write_file(dest, e.data);
            fs::permissions(dest, to_perms(e.mode), fs::perm_options::replace);
            list += "F " + sha256_hex(e.data) + " " + it.rel + "\n";
        }
    }

    // обновление: удаляем файлы, которых в новой версии больше нет
    if (db.installed.count(a.name))
        for (auto& o : read_list(c, a.name))
            if (o.kind != 'D' && !seen.count(o.path)) { std::error_code ec; fs::remove(c.root / o.path, ec); }

    write_file(c.list_path(a.name), list);
    ctl.set("Status", "installed");
    db.installed[a.name] = ctl;
    db.save();
}

static void cmd_install(Ctx& c, const std::vector<std::string>& names) {
    if (names.empty()) throw Error("install: no packages given");
    Db db(c); auto av = load_available(c);
    std::vector<const Avail*> order; std::set<std::string> visiting, done;
    for (auto& n : names) {
        Dep d; d.name = n;
        size_t eq = n.find('=');                       // pkg=1.2.3
        if (eq != std::string::npos) { d.name = n.substr(0, eq); d.op = "="; d.ver = n.substr(eq + 1); }
        plan_visit(d, db, av, order, visiting, done, true);
    }
    if (order.empty()) { std::cout << "Nothing to do.\n"; return; }
    std::cout << "The following packages will be installed:\n ";
    for (auto* a : order) std::cout << " " << a->name << " (" << a->version << ")";
    std::cout << "\n";
    if (!confirm(c)) { std::cout << "Abort.\n"; return; }
    for (auto* a : order) {
        std::cout << "Installing " << a->name << " (" << a->version << ") ...\n";
        install_one(c, db, *a);
    }
    std::cout << "Done: " << order.size() << " package(s) installed.\n";
}

// ===================================================================== удаление
static void cmd_remove(Ctx& c, const std::vector<std::string>& names) {
    if (names.empty()) throw Error("remove: no packages given");
    Db db(c);
    std::set<std::string> gone(names.begin(), names.end());
    for (auto& n : names)
        if (!db.installed.count(n)) throw Error("package '" + n + "' is not installed");

    for (auto& kv : db.installed) {                       // нельзя ломать зависимости
        if (gone.count(kv.first)) continue;
        for (auto& alt : parse_depends(kv.second.value("Depends"))) {
            bool needs = false, other = false;
            for (auto& d : alt) {
                if (gone.count(d.name)) needs = true;
                else if (db.installed.count(d.name)) other = true;
            }
            if (needs && !other && !c.force)
                throw Error("cannot remove: " + kv.first + " depends on " + alt[0].name + " (use --force)");
        }
    }
    std::cout << "The following packages will be removed:\n ";
    for (auto& n : names) std::cout << " " << n;
    std::cout << "\n";
    if (!confirm(c)) { std::cout << "Abort.\n"; return; }

    for (auto& n : names) {
        auto list = read_list(c, n);
        std::set<std::string> shared_dirs;
        for (auto& kv : db.installed) if (kv.first != n)
            for (auto& e : read_list(c, kv.first)) if (e.kind == 'D') shared_dirs.insert(e.path);
        std::vector<std::string> dirs;
        for (auto& e : list) {
            if (e.kind == 'D') { dirs.push_back(e.path); continue; }
            std::error_code ec; fs::remove(c.root / e.path, ec);
        }
        std::sort(dirs.begin(), dirs.end(), [](const std::string& a, const std::string& b) { return a.size() > b.size(); });
        for (auto& d : dirs) if (!shared_dirs.count(d)) { std::error_code ec; fs::remove(c.root / d, ec); }
        std::error_code ec; fs::remove(c.list_path(n), ec);
        db.installed.erase(n);
        db.save();
        std::cout << "Removed " << n << "\n";
    }
}

// ===================================================================== update / запросы
static std::vector<fs::path> source_repos(const Ctx& c) {
    std::vector<fs::path> r = c.repos;
    fs::path src = c.root / "etc/solpkg/sources.list";
    if (fs::exists(src)) {
        std::istringstream in(read_file(src)); std::string line;
        while (std::getline(in, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;
            if (line.rfind("repo ", 0) == 0) line = trim(line.substr(5));
            if (line.rfind("file://", 0) == 0) line = line.substr(7);
            else if (line.find("://") != std::string::npos) throw Error("unsupported repository scheme: " + line + " (only local paths for now)");
            r.push_back(line);
        }
    }
    return r;
}

static void cmd_update(Ctx& c) {
    auto repos = source_repos(c);
    if (repos.empty()) throw Error("no repositories configured (use --repo DIR or etc/solpkg/sources.list)");
    std::string merged; size_t n = 0;
    for (auto& r : repos) {
        fs::path idx = r / "Packages";
        if (!fs::exists(idx)) throw Error("repository has no Packages index: " + r.string());
        for (auto& s : parse_stanzas(read_file(idx))) {
            s.set("Repo", fs::absolute(r).string());
            merged += s.str() + "\n"; n++;
        }
    }
    write_file(c.avail_path(), merged);
    std::cout << "Updated package lists: " << n << " package(s) from " << repos.size() << " repositor" << (repos.size() == 1 ? "y" : "ies") << "\n";
}

static void cmd_list(Ctx& c) {
    Db db(c);
    if (c.installed_only) {
        for (auto& kv : db.installed)
            std::cout << "ii  " << kv.first << "  " << kv.second.value("Version") << "  " << kv.second.value("Description") << "\n";
        return;
    }
    for (auto& a : load_available(c)) {
        auto i = db.installed.find(a.name);
        const char* st = (i != db.installed.end() && i->second.value("Version") == a.version) ? "ii" : "--";
        std::cout << st << "  " << a.name << "  " << a.version << "  " << a.s.value("Description") << "\n";
    }
}

static void cmd_search(Ctx& c, const std::string& term) {
    auto low = [](std::string s) { for (auto& ch : s) ch = (char)std::tolower((unsigned char)ch); return s; };
    std::string t = low(term);
    for (auto& a : load_available(c))
        if (low(a.name).find(t) != std::string::npos || low(a.s.value("Description")).find(t) != std::string::npos)
            std::cout << a.name << " (" << a.version << ") - " << a.s.value("Description") << "\n";
}

static void cmd_show(Ctx& c, const std::string& name) {
    Db db(c);
    auto i = db.installed.find(name);
    if (i != db.installed.end()) { std::cout << i->second.str(); return; }
    for (auto& a : load_available(c)) if (a.name == name) { std::cout << a.s.str(); return; }
    throw Error("no such package: " + name);
}

static void cmd_files(Ctx& c, const std::string& name) {
    Db db(c);
    if (!db.installed.count(name)) throw Error("package '" + name + "' is not installed");
    for (auto& e : read_list(c, name)) std::cout << "/" << e.path << (e.kind == 'D' ? "/" : "") << "\n";
}

static int cmd_verify(Ctx& c, std::vector<std::string> names) {
    Db db(c);
    if (names.empty()) for (auto& kv : db.installed) names.push_back(kv.first);
    int bad = 0; size_t checked = 0;
    for (auto& n : names) {
        if (!db.installed.count(n)) throw Error("package '" + n + "' is not installed");
        for (auto& e : read_list(c, n)) {
            fs::path p = c.root / e.path; std::error_code ec; checked++;
            auto st = fs::symlink_status(p, ec);
            if (ec || !fs::exists(st)) { std::cout << n << ": missing /" << e.path << "\n"; bad++; continue; }
            if (e.kind == 'F') {
                if (!fs::is_regular_file(st)) { std::cout << n << ": not a regular file /" << e.path << "\n"; bad++; continue; }
                if (sha256_hex(read_file(p)) != e.sha) { std::cout << n << ": modified /" << e.path << "\n"; bad++; }
            } else if (e.kind == 'L' && !fs::is_symlink(st)) { std::cout << n << ": not a symlink /" << e.path << "\n"; bad++; }
            else if (e.kind == 'D' && !fs::is_directory(st)) { std::cout << n << ": not a directory /" << e.path << "\n"; bad++; }
        }
    }
    std::cout << "Verified " << checked << " entr" << (checked == 1 ? "y" : "ies") << ", " << bad << " problem(s)\n";
    return bad ? 1 : 0;
}

// ===================================================================== build / index
static void cmd_build(Ctx& c, const std::string& dirarg) {
    fs::path dir = dirarg;
    fs::path ctlp = dir / "SOLPKG/control";
    if (!fs::exists(ctlp)) throw Error("missing " + ctlp.string());
    std::string ctltext = read_file(ctlp);
    auto cs = parse_stanzas(ctltext);
    if (cs.size() != 1) throw Error("control must contain exactly one stanza");
    const Stanza& ctl = cs[0];
    for (const char* req : {"Package", "Version", "Architecture", "Description"})
        if (ctl.value(req).empty()) throw Error(std::string("control: missing field ") + req);
    if (!valid_name(ctl.value("Package"))) throw Error("invalid package name: " + ctl.value("Package"));
    parse_depends(ctl.value("Depends"));    // проверка синтаксиса

    std::vector<TarEntry> es;
    TarEntry ce; ce.path = "SOLPKG/control"; ce.mode = 0644; ce.data = ctltext;
    es.push_back(ce);

    std::vector<fs::path> paths;
    for (auto it = fs::recursive_directory_iterator(dir); it != fs::recursive_directory_iterator(); ++it) {
        fs::path rel = fs::relative(it->path(), dir);
        if (*rel.begin() == "SOLPKG") continue;
        paths.push_back(rel);
    }
    std::sort(paths.begin(), paths.end());
    for (auto& rel : paths) {
        fs::path full = dir / rel;
        auto st = fs::symlink_status(full);
        TarEntry e; e.path = clean_rel(rel.generic_string());
        e.mode = from_perms(st.permissions());
        if (fs::is_directory(st)) e.type = '5';
        else if (fs::is_symlink(st)) { e.type = '2'; e.link = fs::read_symlink(full).generic_string(); }
        else if (fs::is_regular_file(st)) { e.type = '0'; e.data = read_file(full); }
        else throw Error("unsupported file type: " + full.string());
        es.push_back(std::move(e));
    }
    std::string out = c.out.empty()
        ? ctl.value("Package") + "_" + ctl.value("Version") + "_" + ctl.value("Architecture") + ".solpkg" : c.out;
    write_file(out, tar_write(es));
    std::cout << "Built " << out << " (" << es.size() - 1 << " entries)\n";
}

static void cmd_index(Ctx&, const std::string& dirarg) {
    fs::path dir = dirarg;
    std::vector<fs::path> pkgs;
    for (auto& e : fs::recursive_directory_iterator(dir))
        if (e.is_regular_file() && e.path().extension() == ".solpkg") pkgs.push_back(e.path());
    std::sort(pkgs.begin(), pkgs.end());
    std::string out; size_t n = 0;
    for (auto& p : pkgs) {
        std::string blob = read_file(p);
        auto es = tar_read(blob);
        if (es.empty() || es[0].path != "SOLPKG/control") throw Error("not a valid package: " + p.string());
        auto cs = parse_stanzas(es[0].data);
        if (cs.size() != 1) throw Error("bad control in " + p.string());
        Stanza s = cs[0];
        s.set("Filename", fs::relative(p, dir).generic_string());
        s.set("Size", std::to_string(blob.size()));
        s.set("SHA256", sha256_hex(blob));
        out += s.str() + "\n"; n++;
    }
    write_file(dir / "Packages", out);
    std::cout << "Indexed " << n << " package(s) -> " << (dir / "Packages").string() << "\n";
}

// ===================================================================== CLI
static void usage() {
    std::cout <<
"solpkg " << VERSION << " - SolverUNIX package manager\n\n"
"usage: solpkg [options] <command> [args]\n\n"
"commands:\n"
"  update                    read repository indexes into the package lists\n"
"  install <pkg[=ver]>...    install packages with dependencies\n"
"  remove <pkg>...           remove packages\n"
"  list [-i]                 list available (or only installed) packages\n"
"  search <text>             search names and descriptions\n"
"  show <pkg>                show package metadata\n"
"  files <pkg>               list files of an installed package\n"
"  verify [pkg...]           check installed files against recorded SHA-256\n"
"  build <dir> [-o file]     build a .solpkg from a directory with SOLPKG/control\n"
"  index <repo-dir>          create <repo-dir>/Packages\n\n"
"options:\n"
"  --root DIR     operate on the system under DIR (required on a Linux host)\n"
"  --repo DIR     add a repository (local directory)\n"
"  -y, --yes      do not ask for confirmation\n"
"  --force        allow removing packages that others depend on\n"
"  -i             (list) installed packages only\n"
"  -o FILE        (build) output file\n";
}

int main(int argc, char** argv) {
    try {
        Ctx c; std::vector<std::string> pos;
        for (int i = 1; i < argc; i++) {
            std::string a = argv[i];
            auto next = [&]() -> std::string { if (i + 1 >= argc) throw Error("option " + a + " needs an argument"); return argv[++i]; };
            if (a == "--root") { c.root = next(); c.root_explicit = true; }
            else if (a == "--repo") c.repos.push_back(next());
            else if (a == "-y" || a == "--yes") c.yes = true;
            else if (a == "--force") c.force = true;
            else if (a == "-i") c.installed_only = true;
            else if (a == "-o") c.out = next();
            else if (a == "-h" || a == "--help") { usage(); return 0; }
            else if (a == "--version") { std::cout << "solpkg " << VERSION << "\n"; return 0; }
            else if (a.size() > 1 && a[0] == '-') throw Error("unknown option " + a);
            else pos.push_back(a);
        }
        if (pos.empty()) { usage(); return 1; }
        std::string cmd = pos[0]; pos.erase(pos.begin());

#if defined(__linux__)
        // Хост - не SolverUNIX: без явного --root мы бы трогали систему разработчика.
        bool touches_root = cmd == "update" || cmd == "install" || cmd == "remove" || cmd == "list" ||
                            cmd == "search" || cmd == "show" || cmd == "files" || cmd == "verify";
        if (touches_root && !c.root_explicit)
            throw Error("refusing to use the host root '/'; pass --root DIR");
#endif
        if (cmd == "update") cmd_update(c);
        else if (cmd == "install") cmd_install(c, pos);
        else if (cmd == "remove") cmd_remove(c, pos);
        else if (cmd == "list") cmd_list(c);
        else if (cmd == "search") { if (pos.empty()) throw Error("search: need a term"); cmd_search(c, pos[0]); }
        else if (cmd == "show") { if (pos.empty()) throw Error("show: need a package"); cmd_show(c, pos[0]); }
        else if (cmd == "files") { if (pos.empty()) throw Error("files: need a package"); cmd_files(c, pos[0]); }
        else if (cmd == "verify") return cmd_verify(c, pos);
        else if (cmd == "build") { if (pos.empty()) throw Error("build: need a directory"); cmd_build(c, pos[0]); }
        else if (cmd == "index") { if (pos.empty()) throw Error("index: need a directory"); cmd_index(c, pos[0]); }
        else if (cmd == "help") usage();
        else throw Error("unknown command '" + cmd + "'");
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "solpkg: error: " << e.what() << "\n";
        return 1;
    }
}
