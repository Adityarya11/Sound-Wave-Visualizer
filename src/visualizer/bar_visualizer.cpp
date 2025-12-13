#include "bar_visualizer.hpp"
#include <cmath>
#include <cstdlib> // For random (demo mode)

BarVisualizer::BarVisualizer(int barCount, float width, float height)
    : m_barCount(barCount), m_width(width), m_height(height)
{
    setupBars();
}

void BarVisualizer::setupBars()
{
    m_bars.clear();
    m_bars.reserve(m_barCount);

    // Calculate width of each bar
    // Leave a small gap between bars (e.g., 2 pixels)
    float gap = 2.0f;
    float totalGapSpace = gap * (m_barCount - 1);
    float barWidth = (m_width - totalGapSpace) / m_barCount;

    for (int i = 0; i < m_barCount; ++i)
    {
        sf::RectangleShape bar;
        bar.setSize({barWidth, 10.0f});                // Default height 10
        bar.setFillColor(sf::Color(0, 255, 255, 200)); // Cyan with slight transparency

        // Position X: calculate based on index
        // Position Y: Start at the bottom (we will grow upwards)
        float xPos = i * (barWidth + gap);
        float yPos = m_height;

        bar.setPosition({xPos, yPos});

        // Set origin to bottom-left so scaling grows upwards
        bar.setOrigin({0, 0});

        m_bars.push_back(bar);
    }
}

void BarVisualizer::setSize(float width, float height)
{
    m_width = width;
    m_height = height;
    setupBars();
}

void BarVisualizer::update(const std::vector<float> &audioData)
{
    // === DEMO MODE (If no audio data provided) ===
    if (audioData.empty())
    {
        for (int i = 0; i < m_barCount; ++i)
        {
            // Generate a fake height using sine waves + random jitter
            // This makes it look like it's "dancing"
            float time = static_cast<float>(clock()) / 1000.0f;
            float wave = std::sin(time * 5.0f + i * 0.5f);
            float height = (wave * 0.5f + 0.5f) * (m_height * 0.8f); // Scale to 80% height

            // Add jitter
            height += (rand() % 20);

            // Ensure we don't go below 2 pixels
            if (height < 2.0f)
                height = 2.0f;

            // SFML rectangles grow DOWN by default.
            // Since we want them to grow UP, we set the size positive
            // but we need to adjust position.
            // EASIER WAY: Set Origin to Bottom-Left (0, height)??
            // Actually, let's keep it simple:
            // Just update the size. Since we set origin to (0,0) earlier
            // and position Y to m_height, we need to draw UPWARDS.
            // In SFML, Y is down. So "Up" is negative height?
            // Let's try negative height.
            m_bars[i].setSize({m_bars[i].getSize().x, -height});
        }
        return;
    }

    // === REAL AUDIO MODE (Coming later) ===
    // We will implement this when we hook up audio
}

void BarVisualizer::draw(sf::RenderWindow &window)
{
    for (const auto &bar : m_bars)
    {
        window.draw(bar);
    }
}