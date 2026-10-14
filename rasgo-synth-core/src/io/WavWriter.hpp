#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <random>

namespace rasgo {

// Metadata embedded in the WAV via a standard RIFF LIST/INFO chunk (INAM,
// IART, ICRD, ICMT). This is a plain, widely-supported metadata mechanism --
// readable by most DAWs and by `exiftool`/`ffprobe` -- so every recorded
// experiment carries its own title, date, and (in the comment field) the
// JSON patch configuration/seed that generated it. That last part matters
// specifically because the engine is built on chaotic generators: the exact
// audio never repeats, but the configuration that produced a given take
// should always be recoverable from the file itself.
struct WavMetadata {
    std::string title;
    std::string artist;
    std::string date;     // e.g. "2026-07-29"
    std::string comment;  // free text -- we put the patch JSON here

    // Optional, plain human-readable text (not JSON) written into its own
    // custom RIFF chunk placed after the audio data -- see writeWavFile.
    // Empty means no such chunk is written at all.
    std::string epitaph;
};

namespace detail {

inline void writeChunkId(std::ofstream &f, const char *id) {
    f.write(id, 4);
}

inline void writeU32(std::ofstream &f, uint32_t v) {
    char bytes[4] = {
        static_cast<char>(v & 0xFF),
        static_cast<char>((v >> 8) & 0xFF),
        static_cast<char>((v >> 16) & 0xFF),
        static_cast<char>((v >> 24) & 0xFF)
    };
    f.write(bytes, 4);
}

inline void writeU16(std::ofstream &f, uint16_t v) {
    char bytes[2] = {
        static_cast<char>(v & 0xFF),
        static_cast<char>((v >> 8) & 0xFF)
    };
    f.write(bytes, 2);
}

// INFO sub-chunks must be padded to an even number of bytes (with a
// trailing NUL if needed), per the RIFF spec.
inline void writeInfoField(std::ofstream &f, const char *fourCC, const std::string &text) {
    std::string padded = text;
    padded.push_back('\0');
    if (padded.size() % 2 != 0) padded.push_back('\0');
    writeChunkId(f, fourCC);
    writeU32(f, static_cast<uint32_t>(padded.size()));
    f.write(padded.data(), static_cast<std::streamsize>(padded.size()));
}

} // namespace detail

// Writes interleaved float samples (range roughly [-1, 1]) as a 24-bit PCM
// WAV file at the given sample rate / channel count, with an embedded
// LIST/INFO metadata chunk. PCM24 is the Studio's archival/default delivery
// format; processing stays float and TPDF is added only at this final
// fixed-point reduction.
inline bool writeWavFile(const std::string &path,
                          const std::vector<float> &interleavedSamples,
                          int numChannels,
                          int sampleRate,
                          const WavMetadata &meta) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    // Convert to packed little-endian 24-bit PCM up front so we know the
    // data chunk size.
    // Quantizing with a bare lround() (as this used to do) produces
    // quantization error that's correlated with the signal -- audible as
    // low-level artifacts, most noticeably in exactly the quiet passages
    // and long decaying tails (reverb, granular, a coda winding down) this
    // engine produces a lot of. Standard fix: dither with triangular
    // (TPDF) noise -- the sum of two independent uniform draws -- before
    // rounding, which decorrelates the quantization error from the signal
    // at the cost of a very slightly higher noise floor. A fixed-seed RNG
    // is fine here (dither doesn't need true entropy, just needs to be
    // decorrelated sample-to-sample within the file).
    std::mt19937 ditherRng(0xD17A2026u);
    std::uniform_real_distribution<float> ditherUnit(-0.5f, 0.5f);
    std::vector<uint8_t> pcm(interleavedSamples.size() * 3u);
    for (size_t i = 0; i < interleavedSamples.size(); ++i) {
        float s = std::clamp(interleavedSamples[i], -1.0f, 1.0f);
        double scaled = static_cast<double>(s) * 8388607.0;
        float dither = ditherUnit(ditherRng) + ditherUnit(ditherRng); // TPDF
        int32_t q = static_cast<int32_t>(std::llround(scaled + dither));
        q = std::clamp(q, -8388608, 8388607);
        uint32_t packed = static_cast<uint32_t>(q) & 0x00FFFFFFu;
        pcm[i * 3u] = static_cast<uint8_t>(packed & 0xFFu);
        pcm[i * 3u + 1u] = static_cast<uint8_t>((packed >> 8u) & 0xFFu);
        pcm[i * 3u + 2u] = static_cast<uint8_t>((packed >> 16u) & 0xFFu);
    }

    const int bitsPerSample = 24;
    const int blockAlign = numChannels * bitsPerSample / 8;
    const int byteRate = sampleRate * blockAlign;
    const uint32_t dataSize = static_cast<uint32_t>(pcm.size());

    // -- RIFF header (size patched in after we know the total) --
    long riffSizePos;
    detail::writeChunkId(f, "RIFF");
    riffSizePos = static_cast<long>(f.tellp());
    detail::writeU32(f, 0); // placeholder
    detail::writeChunkId(f, "WAVE");

    // -- fmt chunk --
    detail::writeChunkId(f, "fmt ");
    detail::writeU32(f, 16);
    detail::writeU16(f, 1); // PCM
    detail::writeU16(f, static_cast<uint16_t>(numChannels));
    detail::writeU32(f, static_cast<uint32_t>(sampleRate));
    detail::writeU32(f, static_cast<uint32_t>(byteRate));
    detail::writeU16(f, static_cast<uint16_t>(blockAlign));
    detail::writeU16(f, static_cast<uint16_t>(bitsPerSample));

    // -- LIST/INFO chunk (metadata) --
    {
        // Precompute each field's padded size.
        auto paddedSize = [](const std::string &s) {
            size_t n = s.size() + 1;
            if (n % 2 != 0) n += 1;
            return n;
        };
        uint32_t listContentSize = 4; // "INFO"
        listContentSize += 8 + static_cast<uint32_t>(paddedSize(meta.title));
        listContentSize += 8 + static_cast<uint32_t>(paddedSize(meta.artist));
        listContentSize += 8 + static_cast<uint32_t>(paddedSize(meta.date));
        listContentSize += 8 + static_cast<uint32_t>(paddedSize(meta.comment));

        detail::writeChunkId(f, "LIST");
        detail::writeU32(f, listContentSize);
        detail::writeChunkId(f, "INFO");
        detail::writeInfoField(f, "INAM", meta.title);
        detail::writeInfoField(f, "IART", meta.artist);
        detail::writeInfoField(f, "ICRD", meta.date);
        detail::writeInfoField(f, "ICMT", meta.comment);
    }

    // -- data chunk --
    detail::writeChunkId(f, "data");
    detail::writeU32(f, dataSize);
    f.write(reinterpret_cast<const char *>(pcm.data()), static_cast<std::streamsize>(dataSize));

    // -- optional epitaph chunk, placed after the audio data on purpose --
    // any WAV reader stops needing bytes once it has consumed `dataSize`
    // worth of the data chunk, and an unrecognised trailing RIFF chunk is
    // explicitly legal per the spec (readers are required to skip chunks
    // they don't understand), so this never touches how the file sounds
    // or whether it opens correctly. Opened in a text/hex editor instead
    // of a DAW, though, the piece's own short, human-readable account of
    // itself sits right there in plain ASCII -- the same "the file
    // documents its own biography" idea meta.comment's JSON already
    // carries, just legible without parsing anything.
    if (!meta.epitaph.empty()) {
        std::string padded = meta.epitaph;
        if (padded.size() % 2 != 0) padded.push_back(' ');
        detail::writeChunkId(f, "epit");
        detail::writeU32(f, static_cast<uint32_t>(padded.size()));
        f.write(padded.data(), static_cast<std::streamsize>(padded.size()));
    }

    long endPos = static_cast<long>(f.tellp());
    if (!f || endPos < riffSizePos + 4) return false;
    uint32_t riffSize = static_cast<uint32_t>(endPos - riffSizePos - 4);
    f.seekp(riffSizePos);
    detail::writeU32(f, riffSize);

    // Buffered writes can fail only at flush/close (e.g. storage exhausted).
    // Never index or announce an incomplete WAV as a successful recording.
    f.flush();
    if (!f) return false;
    f.close();
    return !f.fail();
}

} // namespace rasgo
