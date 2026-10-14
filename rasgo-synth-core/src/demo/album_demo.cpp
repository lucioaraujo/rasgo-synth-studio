// Rasgo Synth Core -- Phase 18 CLI demo (album cohesion mechanism updated
// in fase 83, see AlbumRenderer.hpp).
//
// Thin CLI wrapper around AlbumRenderer.hpp, same relationship
// integrated_piece_demo.cpp has to PieceRenderer.hpp: renders a set of
// tracks sharing a bpm/energy mood (optionally from a named theme) and
// walking a straight-line trajectory across the Caillois game weights,
// while each track picks its own scale and is otherwise fully seed-driven
// -- so a batch reads as one album instead of N unrelated experiments.
//
// Usage: rasgo_album_demo [albumSeed] [trackCount] [outDir] [themeName]

#include <cstdio>
#include <string>

#include "../engine/AlbumRenderer.hpp"

using namespace rasgo;

int main(int argc, char **argv) {
    unsigned albumSeed = (argc > 1) ? static_cast<unsigned>(std::stoul(argv[1])) : 2026u;
    int trackCount = (argc > 2) ? std::stoi(argv[2]) : 8;
    std::string outDir = (argc > 3) ? argv[3] : "recordings";
    std::string themeName = (argc > 4) ? argv[4] : "Livre";

    const AlbumTheme *theme = findAlbumTheme(themeName);
    if (!theme) {
        std::fprintf(stderr, "Tema desconhecido: %s. Temas disponiveis:\n", themeName.c_str());
        for (auto &t : albumThemes()) std::fprintf(stderr, "  - %s\n", t.name);
        return 1;
    }

    AlbumConfig cfg = albumConfigFromTheme(*theme, albumSeed, trackCount, outDir);
    std::fprintf(stderr, "Album seed %u, tema \"%s\", %d faixas -> %s\n",
                 albumSeed, theme->name, trackCount, outDir.c_str());

    auto results = renderAlbum(cfg, [](int trackIdx, int trackCount_, const PieceProgress &p) {
        static int lastPrinted = -1;
        if (trackIdx != lastPrinted) {
            std::fprintf(stderr, "  faixa %d/%d...\n", trackIdx + 1, trackCount_);
            lastPrinted = trackIdx;
        }
        (void)p;
    });

    int failures = 0;
    for (auto &r : results) {
        if (r.piece.path.empty()) {
            std::fprintf(stderr, "  FALHA na faixa (seed %u)\n", r.trackSeed);
            ++failures;
        } else {
            std::printf("%s\n", r.piece.path.c_str());
        }
    }

    if (failures > 0) {
        std::fprintf(stderr, "%d faixa(s) falharam.\n", failures);
        return 1;
    }
    return 0;
}
