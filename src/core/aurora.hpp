#pragma once

#include <SFML/Graphics/Color.hpp>

#include <algorithm>
#include <cmath>

// The palette, shared between C++ geometry (bar mode, chrome) and the GLSL in
// assets/aurora.frag. Keep the two stop lists in sync when retuning colours.
namespace aurora
{

struct Rgb
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};

// Deep indigo -> electric violet -> magenta -> cyan -> mint.
inline Rgb ramp(float t)
{
    static const Rgb stops[5] = {
        {0.106f, 0.031f, 0.259f},
        {0.482f, 0.184f, 0.969f},
        {0.780f, 0.141f, 0.694f},
        {0.133f, 0.827f, 0.933f},
        {0.486f, 1.000f, 0.796f},
    };

    const float x = std::clamp(t, 0.0f, 1.0f) * 4.0f;
    const int i = std::min(3, static_cast<int>(x));
    float f = x - static_cast<float>(i);
    f = f * f * (3.0f - 2.0f * f);

    return {stops[i].r + (stops[i + 1].r - stops[i].r) * f,
            stops[i].g + (stops[i + 1].g - stops[i].g) * f,
            stops[i].b + (stops[i + 1].b - stops[i].b) * f};
}

// Ping-pong rather than wrap: the ramp is not cyclic, so a plain fract() would
// snap mint straight back to indigo and show a hard seam as the hue drifts.
inline Rgb rampCyclic(float t)
{
    const float w = t - std::floor(t);
    return ramp(std::fabs(w * 2.0f - 1.0f));
}

inline sf::Color color(float t, float alpha = 1.0f)
{
    const Rgb c = rampCyclic(t);
    const auto q = [](float v) {
        return static_cast<std::uint8_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
    };
    return sf::Color(q(c.r), q(c.g), q(c.b), q(alpha));
}

} // namespace aurora
