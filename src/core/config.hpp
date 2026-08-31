#pragma once

#include <cstdint>
#include <filesystem>

enum class Mode
{
    Circle = 0,
    Bars = 1,
};

// Persisted between runs so the overlay comes back exactly where you left it.
// Written to %APPDATA%/Visualizer/config.json rather than next to the exe,
// because the exe lives on PATH in a directory that may not be writable.
struct Config
{
    int x = INT32_MIN; // INT32_MIN means "not placed yet, pick a default"
    int y = INT32_MIN;
    unsigned int scale = 340; // orb diameter in pixels; bar mode derives from it
    Mode mode = Mode::Circle;
    float gain = 1.0f;
    float opacity = 0.95f;
    bool fadeWhenSilent = true;

    static Config load();
    void save() const;
};

namespace paths
{
std::filesystem::path exeDirectory();
std::filesystem::path configFile();
std::filesystem::path shaderFile();
} // namespace paths
