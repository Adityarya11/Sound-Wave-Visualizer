#include "core/config.hpp"

#include <algorithm>
#include <fstream>

#include <windows.h>

#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

namespace paths
{

fs::path exeDirectory()
{
    std::wstring buffer(MAX_PATH, L'\0');
    for (;;)
    {
        const DWORD written = GetModuleFileNameW(nullptr, buffer.data(),
                                                 static_cast<DWORD>(buffer.size()));
        if (written == 0)
            return fs::current_path();

        if (written < buffer.size())
        {
            buffer.resize(written);
            break;
        }

        buffer.resize(buffer.size() * 2);
    }

    return fs::path(buffer).parent_path();
}

fs::path configFile()
{
    // GetEnvironmentVariableW rather than the CRT getenv variants: it is
    // always present under MinGW and returns the wide value directly.
    const DWORD needed = GetEnvironmentVariableW(L"APPDATA", nullptr, 0);
    if (needed > 1)
    {
        std::wstring appData(needed, L'\0');
        const DWORD written = GetEnvironmentVariableW(L"APPDATA", appData.data(), needed);
        if (written > 0 && written < needed)
        {
            appData.resize(written);
            return fs::path(appData) / L"Visualizer" / L"config.json";
        }
    }

    return exeDirectory() / L"config.json";
}

fs::path shaderFile()
{
    return exeDirectory() / L"aurora.frag";
}

} // namespace paths

Config Config::load()
{
    Config config;

    std::ifstream file(paths::configFile());
    if (!file)
        return config;

    try
    {
        nlohmann::json json;
        file >> json;

        config.x = json.value("x", config.x);
        config.y = json.value("y", config.y);
        config.scale = std::clamp(json.value("scale", config.scale), 120u, 1400u);
        config.mode = json.value("mode", 0) == 1 ? Mode::Bars : Mode::Circle;
        config.gain = std::clamp(json.value("gain", config.gain), 0.25f, 4.0f);
        config.opacity = std::clamp(json.value("opacity", config.opacity), 0.15f, 1.0f);
        config.fadeWhenSilent = json.value("fadeWhenSilent", config.fadeWhenSilent);
    }
    catch (const std::exception &)
    {
        // A corrupt config should never stop the app starting; defaults win.
        return Config{};
    }

    return config;
}

void Config::save() const
{
    const fs::path file = paths::configFile();

    std::error_code ec;
    fs::create_directories(file.parent_path(), ec);

    std::ofstream out(file);
    if (!out)
        return;

    const nlohmann::json json = {
        {"x", x},
        {"y", y},
        {"scale", scale},
        {"mode", mode == Mode::Bars ? 1 : 0},
        {"gain", gain},
        {"opacity", opacity},
        {"fadeWhenSilent", fadeWhenSilent},
    };

    out << json.dump(2) << '\n';
}
