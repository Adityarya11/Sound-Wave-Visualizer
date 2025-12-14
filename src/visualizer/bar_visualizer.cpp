#include "bar_visualizer.hpp"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>

BarVisualizer::BarVisualizer(int barCount, float width, float height)
    : m_barCount(barCount), m_width(width), m_height(height)
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    m_smoothedValues.resize(barCount, 0.0f);
    setupBars();
}

void BarVisualizer::setupBars()
{
    m_bars.clear();
    m_bars.reserve(m_barCount);

    float gap = 2.0f;
    float totalGapSpace = gap * (m_barCount - 1);
    float barWidth = (m_width - totalGapSpace) / m_barCount;

    for (int i = 0; i < m_barCount; ++i)
    {
        sf::RectangleShape bar;
        bar.setSize({barWidth, 10.0f});
        bar.setFillColor(sf::Color(0, 255, 255, 200));

        float xPos = i * (barWidth + gap);
        bar.setPosition({xPos, m_height});
        m_bars.push_back(bar);
    }
}

void BarVisualizer::setSize(float width, float height)
{
    m_width = width;
    m_height = height;
    setupBars();
}

void BarVisualizer::update(const std::vector<float> &fftData)
{
    // === DEMO MODE (if no FFT data) ===
    if (fftData.empty())
    {
        static float time = 0.0f;
        time += 0.05f;

        for (int i = 0; i < m_barCount; ++i)
        {
            float wave1 = std::sin(time + i * 0.2f);
            float wave2 = std::sin(time * 0.5f + i * 0.1f);
            float intensity = (wave1 + wave2 + 2.0f) / 4.0f;
            float jitter = (static_cast<float>(rand() % 100) / 100.0f) * 0.2f;
            intensity += jitter;

            float barHeight = intensity * (m_height * 0.8f);
            if (barHeight < 5.0f)
                barHeight = 5.0f;

            m_smoothedValues[i] = barHeight;
            sf::Vector2f currentSize = m_bars[i].getSize();
            m_bars[i].setSize({currentSize.x, -barHeight});

            sf::Color color = sf::Color(0, 255, 255, 200);
            if (intensity > 0.8f)
                color = sf::Color(255, 255, 255, 220);
            m_bars[i].setFillColor(color);
        }
        return;
    }

    // === REAL FFT DATA MODE ===
    // Map FFT bins to our bar count
    int fftSize = static_cast<int>(fftData.size());

    for (int i = 0; i < m_barCount; ++i)
    {
        // Map bar index to FFT bin range (logarithmic scaling for better visuals)
        // Lower frequencies get more bars (they contain more musical info)
        float t = static_cast<float>(i) / m_barCount;
        int fftIndex = static_cast<int>(std::pow(t, 2.0f) * (fftSize / 2));
        fftIndex = std::clamp(fftIndex, 0, fftSize - 1);

        // Get the FFT value (0.0 to 1.0)
        float value = fftData[fftIndex];

        // Apply some gain/scaling
        value = std::clamp(value * 2.0f, 0.0f, 1.0f);

        // Smooth the transition (prevents jittery bars)
        float smoothing = 0.3f; // Lower = smoother, Higher = more reactive
        m_smoothedValues[i] = m_smoothedValues[i] * (1.0f - smoothing) + value * smoothing;

        // Calculate bar height
        float barHeight = m_smoothedValues[i] * (m_height * 0.9f);
        if (barHeight < 2.0f)
            barHeight = 2.0f;

        // Update bar
        sf::Vector2f currentSize = m_bars[i].getSize();
        m_bars[i].setSize({currentSize.x, -barHeight});

        // Dynamic color based on intensity
        sf::Color color;
        if (m_smoothedValues[i] > 0.8f)
        {
            color = sf::Color(255, 100, 100, 230); // Red for peaks
        }
        else if (m_smoothedValues[i] > 0.5f)
        {
            color = sf::Color(255, 255, 100, 220); // Yellow for mid
        }
        else
        {
            color = sf::Color(0, 255, 255, 200); // Cyan for low
        }
        m_bars[i].setFillColor(color);
    }
}

void BarVisualizer::draw(sf::RenderWindow &window)
{
    for (const auto &bar : m_bars)
    {
        window.draw(bar);
    }
}