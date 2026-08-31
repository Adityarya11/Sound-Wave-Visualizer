#pragma once

#include <SFML/Graphics.hpp>

#include <vector>

// Everything a visualiser needs for one frame. Passed by const reference so
// visualisers stay pure renderers and never reach back into audio or config.
struct VisualState
{
    const std::vector<float> *bands = nullptr; // 0..1 per log-spaced band
    float level = 0.0f;                        // 0..1 overall loudness
    float beat = 0.0f;                         // 0..1 bass transient impulse
    float time = 0.0f;                         // seconds since launch
    float opacity = 1.0f;                      // master fade, 0 when idle
    float hue = 0.0f;                          // palette rotation, wraps
};

class VisualizerBase
{
public:
    virtual ~VisualizerBase() = default;

    virtual void resize(sf::Vector2f size) = 0;
    virtual void update(const VisualState &state, float dt) = 0;
    virtual void draw(sf::RenderTarget &target) = 0;

    // Preferred window aspect. The app resizes the overlay to suit the mode
    // so the orb is not clipped by a wide, short bar-style window.
    virtual sf::Vector2u preferredSize(unsigned int scale) const = 0;
};
