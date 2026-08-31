#include "visualizer/circle_visualizer.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

#include "aurora_frag.hpp"

CircleVisualizer::CircleVisualizer()
{
    m_quad.setSize(m_size);
    m_pixels.fill(0);
}

bool CircleVisualizer::load(const std::filesystem::path &shaderPath)
{
    if (!sf::Shader::isAvailable())
        return false;

    // A loose aurora.frag beside the exe wins, so the look can be retuned
    // without a rebuild. The baked-in copy is the guaranteed fallback.
    std::string source;
    if (std::ifstream file{shaderPath})
    {
        std::ostringstream buffer;
        buffer << file.rdbuf();
        source = buffer.str();
    }

    if (source.empty() || !m_shader.loadFromMemory(source, sf::Shader::Type::Fragment))
    {
        if (!m_shader.loadFromMemory(assets::kAuroraFrag, sf::Shader::Type::Fragment))
            return false;
    }

    if (!m_spectrum.resize({static_cast<unsigned int>(kTextureBands), 1u}))
        return false;

    // Linear filtering across the row is what turns 64 discrete bands into the
    // continuous undulating rim; clamping stops band 63 bleeding into band 0.
    m_spectrum.setSmooth(true);
    m_spectrum.setRepeated(false);

    m_ready = true;
    return true;
}

void CircleVisualizer::resize(sf::Vector2f size)
{
    m_size = size;
    m_quad.setSize(size);
}

void CircleVisualizer::uploadSpectrum(const std::vector<float> &bands)
{
    const int count = static_cast<int>(bands.size());

    for (int i = 0; i < kTextureBands; ++i)
    {
        float v = 0.0f;
        if (count > 0)
        {
            // Resample whatever band count the analyser produced onto the
            // fixed texture width.
            const float t = static_cast<float>(i) / (kTextureBands - 1);
            const float x = t * (count - 1);
            const int a = static_cast<int>(x);
            const int b = std::min(a + 1, count - 1);
            v = bands[a] + (bands[b] - bands[a]) * (x - static_cast<float>(a));
        }

        const auto q = static_cast<std::uint8_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
        m_pixels[i * 4 + 0] = q;
        m_pixels[i * 4 + 1] = q;
        m_pixels[i * 4 + 2] = q;
        m_pixels[i * 4 + 3] = 255;
    }

    m_spectrum.update(m_pixels.data());
}

void CircleVisualizer::update(const VisualState &state, float)
{
    if (!m_ready)
        return;

    static const std::vector<float> empty;
    uploadSpectrum(state.bands != nullptr ? *state.bands : empty);

    m_shader.setUniform("u_resolution", sf::Glsl::Vec2(m_size));
    m_shader.setUniform("u_time", state.time);
    m_shader.setUniform("u_beat", state.beat);
    m_shader.setUniform("u_level", state.level);
    m_shader.setUniform("u_opacity", state.opacity);
    m_shader.setUniform("u_hue", state.hue);
    m_shader.setUniform("u_spec", m_spectrum);
}

void CircleVisualizer::draw(sf::RenderTarget &target)
{
    if (!m_ready)
        return;

    sf::RenderStates states;
    states.shader = &m_shader;
    target.draw(m_quad, states);
}
