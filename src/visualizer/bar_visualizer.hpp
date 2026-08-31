#pragma once

#include <SFML/Graphics.hpp>

#include <vector>

#include "visualizer/visualizer_base.hpp"

// Linear spectrum bars, mirrored around the centre so the lowest bands sit in
// the middle and rise outward. The old version indexed FFT bins directly with
// a squared curve, which piled most bars onto the first handful of bass bins
// and left the right-hand side permanently flat.
class BarVisualizer : public VisualizerBase
{
public:
    BarVisualizer();

    void resize(sf::Vector2f size) override;
    void update(const VisualState &state, float dt) override;
    void draw(sf::RenderTarget &target) override;

    sf::Vector2u preferredSize(unsigned int scale) const override
    {
        return {scale * 3u, scale * 3u / 4u};
    }

private:
    sf::Vector2f m_size{960.0f, 240.0f};
    sf::VertexArray m_vertices{sf::PrimitiveType::Triangles};
    std::vector<float> m_heights;
};
