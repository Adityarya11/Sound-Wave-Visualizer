#pragma once

#include <SFML/Graphics.hpp>

#include <array>
#include <filesystem>

#include "visualizer/visualizer_base.hpp"

// The NCS-style orb. Unlike the bar mode this draws no per-band geometry:
// the whole thing is one full-window shader pass, which is the only way to get
// soft bloom. Rotated rectangles can only ever produce hard edges.
class CircleVisualizer : public VisualizerBase
{
public:
    CircleVisualizer();

    // Looks for aurora.frag next to the executable and falls back to the copy
    // compiled into the binary. Returns false only if the GPU has no shader
    // support at all, in which case the app switches to bar mode.
    bool load(const std::filesystem::path &shaderPath);

    void resize(sf::Vector2f size) override;
    void update(const VisualState &state, float dt) override;
    void draw(sf::RenderTarget &target) override;

    sf::Vector2u preferredSize(unsigned int scale) const override { return {scale, scale}; }

    bool ready() const { return m_ready; }

private:
    static constexpr int kTextureBands = 64;

    void uploadSpectrum(const std::vector<float> &bands);

    sf::Shader m_shader;
    sf::Texture m_spectrum;
    sf::RectangleShape m_quad;
    std::array<std::uint8_t, kTextureBands * 4> m_pixels{};

    sf::Vector2f m_size{320.0f, 320.0f};
    bool m_ready = false;
};
