#pragma once
// Rasgo Synth Studio — versão de entrega (streaming) da peça renderizada.
// Copyright (c) 2026 Lucio de Araujo — AGPL-3.0-or-later.
//
// Master de entrega (16/11/2026, saida-ref 1.1; o autor: "nivele todos os
// instrumentos para que fiquem preparados para os spotifys da vida"). Ao lado
// do WAV da peça (que segue como sempre: proteção de pico, sem alvo de
// loudness), <peça>_streaming.wav a -14 LUFS / <= -1 dBTP e um .txt com as
// medidas. Escrito pelo writeWavFile do próprio Studio (PCM 24 com TPDF).
// Offline, num fio próprio; o som e a peça não mudam.
#include "../../rasgo-synth-core/src/io/WavWriter.hpp"
#include "saida/DeliveryMaster.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace rasgo {

inline void writeStreamingVersion(const std::string& piecePath, const std::vector<float>& interleaved,
                                  int sampleRate) {
    if (piecePath.empty() || interleaved.size() < 2 || sampleRate <= 0) return;
    struct InterleavedSource final : saida::DeliveryMaster::Source {
        const std::vector<float>& v;
        std::size_t frame = 0;
        explicit InterleavedSource(const std::vector<float>& x) : v(x) {}
        bool rewind() override { frame = 0; return true; }
        std::size_t read(float* l, float* r, std::size_t frames) override {
            const std::size_t n = std::min(frames, v.size() / 2 - frame);
            for (std::size_t i = 0; i < n; ++i) { l[i] = v[2 * (frame + i)]; r[i] = v[2 * (frame + i) + 1]; }
            frame += n;
            return n;
        }
    } source(interleaved);
    struct InterleavedSink final : saida::DeliveryMaster::Sink {
        std::vector<float> out;
        bool write(const float* l, const float* r, std::size_t frames) override {
            for (std::size_t i = 0; i < frames; ++i) { out.push_back(l[i]); out.push_back(r[i]); }
            return true;
        }
    } sink;
    sink.out.reserve(interleaved.size());
    const auto report = saida::DeliveryMaster::process(source, static_cast<double>(sampleRate), sink);
    if (report.failed || sink.out.empty()) return;
    const auto piece = std::filesystem::u8path(piecePath);
    const auto wav = piece.parent_path() / std::filesystem::u8path(piece.stem().u8string() + "_streaming.wav");
    WavMetadata meta;
    meta.title = "Rasgo Synth Studio (streaming)";
    meta.artist = "Lucio de Araujo";
    meta.comment = "Versao de entrega: -14 LUFS, true peak <= -1 dBTP (saida-ref 1.2)";
    if (!writeWavFile(wav.u8string(), sink.out, 2, sampleRate, meta)) return;
    std::ofstream text(std::filesystem::path(wav).replace_extension(".txt"));
    text << "Rasgo Synth Studio - versao de entrega (streaming), saida-ref 1.2\n"
         << "peca: " << piece.filename().u8string() << "\n"
         << "alvo: -14 LUFS, true peak <= -1 dBTP; limitador no maximo 6 dB (guarda de dinamica)\n"
         << "entrada: " << report.inputLufs << " LUFS, " << report.inputTruePeakDb << " dBTP\n"
         << "ganho: " << report.gainDb << " dB; limitador: " << report.limiterReductionDb << " dB\n"
         << "saida: " << report.outputLufs << " LUFS, " << report.outputTruePeakDb << " dBTP\n"
         << (report.silent ? "silencio: copiada como veio\n"
             : report.reachedTarget ? "alvo atingido\n"
             : "alvo nao atingido sem esmagar a dinamica: entregue no mais alto possivel (os servicos sobem faixas baixas no modo normal)\n");
}

} // namespace rasgo
