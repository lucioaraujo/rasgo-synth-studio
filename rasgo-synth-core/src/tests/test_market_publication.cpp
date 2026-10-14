#include "../engine/ProbabilityMarket.hpp"
#include "../io/PublishedIndex.hpp"
#include <cmath>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>
#ifdef __unix__
#include <csignal>
#include <sys/resource.h>
#endif
namespace fs = std::filesystem;
static void require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
static std::string bytes(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
static void write(const fs::path& path, const std::string& text) {
    std::ofstream file(path, std::ios::binary); file << text; file.close();
    require(file.good(), "fixture write failed");
}
static bool refused(const rasgo::ProbabilityMarket& market, const fs::path& target) {
    try { market.save(target.string()); }
    catch (const std::runtime_error& error) {
        require(std::string(error.what()).find(target.string()) != std::string::npos, "missing recovery target");
        return true;
    }
    return false;
}
int main(int argc, char** argv) {
    if (argc > 2) return 2;
    const auto root = argc == 2 ? fs::path(argv[1]) : fs::current_path()
        / ("market-publication-" + rasgo::RecordingPublication::uniqueToken());
    if (fs::exists(root) || !fs::create_directory(root)) return 2;
    try {
        rasgo::ProbabilityMarket market;
        market.price("alpha", .25); market.price("zeta", .75);
        const auto normal = root / "market.txt";
        market.save(normal.string());
        require(bytes(normal) == "alpha=0.25\nzeta=0.75\n", "serialization changed");
        rasgo::ProbabilityMarket loaded; loaded.load(normal.string());
        require(loaded.price("alpha", .5) == .25 && loaded.price("zeta", .5) == .75, "roundtrip");
        market.price("alpha", .5); // existing price must stay unchanged
        market.save(normal.string());
        require(bytes(normal) == "alpha=0.25\nzeta=0.75\n", "replacement bytes");
        const auto occupied = root / "occupied.txt";
        fs::create_directory(occupied); write(occupied / "sentinel", "keep\n");
        require(refused(market, occupied), "silent market save failure");
        require(bytes(occupied / "sentinel") == "keep\n", "directory changed");
        const auto old = bytes(normal);
        fs::permissions(normal, fs::perms::owner_read);
        require(refused(market, normal) && bytes(normal) == old, "readonly changed");
        fs::permissions(normal, fs::perms::owner_read | fs::perms::owner_write);
        market.save(normal.string());
        require(fs::status(normal).permissions() == (fs::perms::owner_read | fs::perms::owner_write), "permissions changed");
        require(refused(market, root / "missing/market.txt"), "missing parent accepted");
        const auto lock = root / ".market.txt.lock";
        fs::create_directory(lock);
        const auto started = std::chrono::steady_clock::now();
        require(refused(market, normal) && bytes(normal) == old && fs::is_directory(lock), "retained lock changed");
        require(std::chrono::steady_clock::now() - started < std::chrono::seconds(5), "unbounded lease wait");
        fs::remove(lock); // exact own fixture; never an application lock
        const auto large = root / "large.txt";
        write(large, std::string(65537, 'x'));
        require(refused(market, large) && fs::file_size(large) == 65537, "oversized target changed");
        const auto invalid = root / "invalid.txt";
        write(invalid, "bad=nan\nbad2=inf\ngood=0.42\n");
        rasgo::ProbabilityMarket parsed; parsed.load(invalid.string());
        require(parsed.price("bad", .5) == .5 && parsed.price("bad2", .5) == .5
                && parsed.price("good", .5) == .42, "nonfinite prices accepted");
        rasgo::ProbabilityMarket invalidKey;
        invalidKey.price("name\ninjection", .5);
        require(refused(invalidKey, normal) && bytes(normal) == old, "invalid key accepted");
#ifdef __unix__
        fs::create_symlink(fs::absolute(normal), root / "link.txt");
        require(refused(market, root / "link.txt") && bytes(normal) == old, "symlink changed");
        const auto alias = root / "alias.txt";
        fs::create_hard_link(normal, alias);
        require(refused(market, normal) && bytes(normal) == old, "hardlink changed");
        fs::remove(alias); // exact own fixture alias
        rlimit previous{}; require(::getrlimit(RLIMIT_FSIZE, &previous) == 0, "getrlimit");
        auto limited = previous; limited.rlim_cur = 16;
        const auto handler = std::signal(SIGXFSZ, SIG_IGN);
        require(::setrlimit(RLIMIT_FSIZE, &limited) == 0, "setrlimit");
        const bool failure = refused(market, normal);
        require(::setrlimit(RLIMIT_FSIZE, &previous) == 0, "restore rlimit");
        std::signal(SIGXFSZ, handler);
        require(failure && bytes(normal) == old, "write failure corrupted market");
#endif
        for (const auto& item : fs::directory_iterator(root)) {
            const auto name = item.path().filename().string();
            require(name.find(".pending-") == std::string::npos && name.find(".lock") == std::string::npos, "owned staging leaked");
        }
        std::cout << "PASS market publication: exact serialization, preserved old bytes/permissions, explicit failures, bounded lease/load, refused links/nonfinite input\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n'; return 1;
    }
}
