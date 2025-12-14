#include "bar_visualizer.hpp"
#include <algorithm>
#include <cstdint>

BarVisualizer::BarVisualizer(int barCount, float width, float height)
    : m_barCount(barCount), m_width(width), m_height(height)
{
    m_smoothHeights.resize(barCount, 0.0f);

    // Create shapes
    float barWidth = m_width / m_barCount;
    for (int i = 0; i < m_barCount; i++)
    {
        sf::RectangleShape rect(sf::Vector2f(barWidth - 2.0f, 10.0f));
        rect.setFillColor(sf::Color(0, 255, 255, 200)); // Cyan
        m_bars.push_back(rect);
    }
}

void BarVisualizer::update(const std::vector<float> &frequencyData)
{
    if (frequencyData.empty())
        return;

    // Map frequency bins to bars (simple method)
    // We typically ignore the high end (it's often empty in music)
    int step = frequencyData.size() / m_barCount;
    if (step < 1)
        step = 1;

    for (int i = 0; i < m_barCount; i++)
    {
        int index = i * step;
        if (index >= frequencyData.size())
            break;

        float target = frequencyData[index];

        // Smoothing (Linear Interpolation)
        if (target > m_smoothHeights[i])
        {
            m_smoothHeights[i] = target; // Jump up fast
        }
        else
        {
            m_smoothHeights[i] -= 0.02f; // Fall down slow
        }
        if (m_smoothHeights[i] < 0)
            m_smoothHeights[i] = 0;

        // Update Rect
        float h = m_smoothHeights[i] * m_height;
        if (h > m_height)
            h = m_height;

        m_bars[i].setSize(sf::Vector2f(m_bars[i].getSize().x, h));
        m_bars[i].setPosition({i * (m_width / m_barCount), m_height - h});

        // Dynamic Color (Low = Blue, High = Purple)
        std::uint8_t r = static_cast<std::uint8_t>(m_smoothHeights[i] * 255);
        m_bars[i].setFillColor(sf::Color(r, 100, 255, 200));
    }
}

void BarVisualizer::draw(sf::RenderWindow &window)
{
    for (const auto &bar : m_bars)
    {
        window.draw(bar);
    }
}