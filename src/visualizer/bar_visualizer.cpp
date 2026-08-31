#include "visualizer/bar_visualizer.hpp"

#include <algorithm>
#include <cmath>

#include "core/aurora.hpp"

namespace
{
// Fraction of the window height where the bars stand. The remainder below
// carries the reflection.
constexpr float kBaselineFraction = 0.66f;
constexpr float kReflectionScale = 0.45f;
constexpr float kGapFraction = 0.22f; // of one bar slot
} // namespace

BarVisualizer::BarVisualizer() = default;

void BarVisualizer::resize(sf::Vector2f size)
{
    m_size = size;
}

void BarVisualizer::update(const VisualState &state, float)
{
    const std::vector<float> *bands = state.bands;
    if (bands == nullptr || bands->empty())
    {
        m_vertices.clear();
        return;
    }

    const int bandCount = static_cast<int>(bands->size());
    const int barCount = bandCount * 2;

    m_heights.resize(barCount);
    for (int i = 0; i < barCount; ++i)
    {
        // Mirror around the centre: the two innermost bars are the lowest
        // band, and pitch rises outward in both directions.
        const int band = i < bandCount ? (bandCount - 1 - i) : (i - bandCount);
        m_heights[i] = (*bands)[band];
    }

    const float slot = m_size.x / static_cast<float>(barCount);
    const float gap = slot * kGapFraction;
    const float barWidth = std::max(1.0f, slot - gap);
    const float baseline = m_size.y * kBaselineFraction;
    const float maxHeight = baseline * 0.94f;

    m_vertices.clear();
    m_vertices.resize(static_cast<std::size_t>(barCount) * 12);

    std::size_t v = 0;
    const auto quad = [&](float x0, float x1, float yTop, float yBottom,
                          sf::Color top, sf::Color bottom) {
        m_vertices[v + 0] = sf::Vertex{{x0, yTop}, top};
        m_vertices[v + 1] = sf::Vertex{{x1, yTop}, top};
        m_vertices[v + 2] = sf::Vertex{{x1, yBottom}, bottom};
        m_vertices[v + 3] = sf::Vertex{{x0, yTop}, top};
        m_vertices[v + 4] = sf::Vertex{{x1, yBottom}, bottom};
        m_vertices[v + 5] = sf::Vertex{{x0, yBottom}, bottom};
        v += 6;
    };

    for (int i = 0; i < barCount; ++i)
    {
        const float value = std::clamp(m_heights[i], 0.0f, 1.0f);
        const float height = std::max(2.0f, value * maxHeight);

        const float x0 = static_cast<float>(i) * slot + gap * 0.5f;
        const float x1 = x0 + barWidth;
        const float yTop = baseline - height;

        // Hue walks outward from the centre and brightens with amplitude, so
        // the whole strip reads as one gradient instead of 128 separate bars.
        const float spread = std::fabs(static_cast<float>(i) / (barCount - 1) - 0.5f) * 2.0f;
        const float hue = state.hue + spread * 0.30f + value * 0.28f;

        const sf::Color top = aurora::color(hue, (0.55f + 0.45f * value) * state.opacity);
        const sf::Color bottom = aurora::color(hue - 0.12f, 0.22f * state.opacity);

        quad(x0, x1, yTop, baseline, top, bottom);

        const float reflectTop = baseline + m_size.y * 0.012f;
        const float reflectBottom = std::min(m_size.y, reflectTop + height * kReflectionScale);
        quad(x0, x1, reflectTop, reflectBottom,
             aurora::color(hue, 0.26f * value * state.opacity),
             aurora::color(hue, 0.0f));
    }
}

void BarVisualizer::draw(sf::RenderTarget &target)
{
    if (m_vertices.getVertexCount() > 0)
        target.draw(m_vertices);
}
