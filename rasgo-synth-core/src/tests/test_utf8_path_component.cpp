#include "../io/Utf8PathComponent.hpp"
#include "../io/WavWriter.hpp"
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
    if (argc > 2) return 2;
    int failures = 0;
    const auto check = [&](bool valid) { if (!valid) ++failures; };
    check(rasgo::utf8PathComponent(u8"Álbum — criação, rêverie e coração") == u8"Álbum — criação_ rêverie e coração");
    check(rasgo::utf8PathComponent("Album 42-test") == "Album 42-test");
    check(rasgo::utf8PathComponent("../bad\\path\n") == "___bad_path_");
    check(rasgo::utf8PathComponent(u8"a\u0301") == u8"a\u0301");
    check(rasgo::utf8PathComponent(u8"áé", 3) == u8"á");
    check(rasgo::utf8PathComponent(u8"🎶", 3).empty());
    check(rasgo::utf8PathComponent(u8"🎶", 4) == u8"🎶");
    check(rasgo::utf8PathComponent(std::string(500, 'a')).size() == 96);
    check(rasgo::utf8PathComponent("a", 0).empty());
    for (const auto& invalid : {std::string("\xc0\xaf"), std::string("\xed\xa0\x80"),
            std::string("\xf4\x90\x80\x80"), std::string("\xe2\x82"), std::string("\x80")})
        check(rasgo::utf8PathComponent(invalid) == std::string(invalid.size(), '_'));
    check(rasgo::utf8PathComponent("\xc2\x85") == "_");
    // Optional tiny integration fixture. Never reuse an existing destination;
    // the normal CTest remains file-free. Preserve evidence for an independent
    // metadata decoder rather than removing it in the test.
    if (argc == 2) {
        const std::filesystem::path root(argv[1]);
        if (!std::filesystem::create_directory(root)) return 2;
        const auto folder = root / rasgo::utf8PathComponent(u8"Álbum — coração e rêverie");
        check(std::filesystem::create_directory(folder));
        const auto path = folder / u8"Canção — été.wav";
        rasgo::WavMetadata metadata;
        metadata.title = u8"Álbum — coração e rêverie";
        metadata.artist = u8"Lúcio — création";
        metadata.comment = "UTF-8 fixture, silence only; not a musical render";
        check(rasgo::writeWavFile(path.string(), std::vector<float>(512, 0.0f), 2, 48000, metadata));
        check(std::filesystem::is_regular_file(path));
        std::cout << path.string() << '\n';
    }
    std::cout << "UTF-8 path checks: " << failures << " failures\n";
    return failures ? 1 : 0;
}
