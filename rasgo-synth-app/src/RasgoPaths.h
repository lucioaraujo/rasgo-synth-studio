// Rasgo Synth App -- portable runtime paths.
//
// Source checkouts, packaged applications and automated tests must not depend
// on the development machine's absolute directory layout. Environment
// overrides are intentionally supported for packaging and isolated tests.

#pragma once

#include <juce_core/juce_core.h>

namespace rasgo::paths {

inline juce::File environmentDirectory(const char *name) {
    const auto value = juce::SystemStats::getEnvironmentVariable(name, {});
    return value.isNotEmpty() ? juce::File(value) : juce::File{};
}

inline juce::File appDataDirectory() {
    auto overrideDirectory = environmentDirectory("RASGO_SYNTH_DATA_DIR");
    if (overrideDirectory != juce::File{}) return overrideDirectory;

    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Rasgo Synth");
}

inline juce::File recordingsDirectory() {
    auto overrideDirectory = environmentDirectory("RASGO_SYNTH_RECORDINGS_DIR");
    if (overrideDirectory != juce::File{}) return overrideDirectory;

    return juce::File::getSpecialLocation(juce::File::userMusicDirectory)
        .getChildFile("Rasgo Synth");
}

inline juce::File logsDirectory() {
    return appDataDirectory().getChildFile("Logs");
}

inline juce::File consoleLogFile() {
    return logsDirectory().getChildFile("rasgo_synth_console.log");
}

inline juce::File structuredLogFile() {
    return logsDirectory().getChildFile("rasgo_synth.log");
}

inline juce::File artistPresetFile() {
    return appDataDirectory().getChildFile("artist_preset.txt");
}

inline juce::File assetsDirectory() {
    auto overrideDirectory = environmentDirectory("RASGO_SYNTH_ASSETS_DIR");
    if (overrideDirectory != juce::File{}) return overrideDirectory;

    const auto executable = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
    const auto executableDirectory = executable.getParentDirectory();
    const juce::File candidates[] = {
        executableDirectory.getChildFile("assets"),
        executableDirectory.getParentDirectory().getChildFile("Resources").getChildFile("assets"),
        executableDirectory.getParentDirectory().getChildFile("share").getChildFile("rasgo-synth").getChildFile("assets")
    };

    for (const auto &candidate : candidates)
        if (candidate.isDirectory()) return candidate;

    // A stable expected location gives callers a useful missing-file path
    // while retaining their existing graceful fallback behaviour.
    return candidates[0];
}

inline juce::File assetFile(const juce::String &relativePath) {
    return assetsDirectory().getChildFile(relativePath);
}

} // namespace rasgo::paths
