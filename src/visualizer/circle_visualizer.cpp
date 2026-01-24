#include "circle_visualizer.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CircleVisualiser::CircleVisualiser(int barCount, float baseRadius, sf::Vector2f centerPosition)
    : m_barCount(barCount), m_baseRadius(baseRadius), m_center(centerPosition)
{
    m_smoothedValues.resize(barCount, 0.0f);
    setupBars();
}

void CircleVisualiser::setupBars()
{
    m_bars.clear();
    m_bars.reserve(m_barCount);

    // We want the circle to be perfectly distributed over 360 degrees
    float angleStep = 360.0f / m_barCount;

    for (int i = 0; i < m_barCount; ++i)
    {
        sf::RectangleShape bar;
        // Width is fixed, height will change with music
        bar.setSize({4.0f, 10.0f});

        // Origin at the "bottom center" of the bar so it grows outward from the circle rim
        bar.setOrigin({2.0f, 0.0f});

        // Initial position (will be updated in loop, but setting rotation here matters)
        bar.setRotation(sf::degrees(i * angleStep));

        m_bars.push_back(bar);
    }
}

void CircleVisualiser::setCenter(sf::Vector2f center)
{
    m_center = center;
}

void CircleVisualiser::update(const std::vector<float> &fftData)
{
    int fftSize = static_cast<int>(fftData.size());

    // --- DEMO MODE (No Audio) ---
    if (fftData.empty())
    {
        static float time = 0.0f;
        time += 0.02f;
        for (int i = 0; i < m_barCount; ++i)
        {
            float val = (std::sin(time * 5.0f + i * 0.5f) + 1.0f) * 0.5f;
            m_smoothedValues[i] = val * 50.0f;
        }
    }
    // --- REAL MODE ---
    else
    {
        // We want to mirror the spectrum to make it look symmetrical (NCS style)
        // Left side of circle = low->high, Right side = high->low (or vice versa)
        int halfBars = m_barCount / 2;

        for (int i = 0; i < halfBars; ++i)
        {
            // Logarithmic index mapping
            float t = (float)i / halfBars;
            int fftIndex = static_cast<int>(std::pow(t, 2.0f) * (fftSize / 4)); // Use lower quarter of FFT for bass
            fftIndex = std::clamp(fftIndex, 0, fftSize - 1);

            float value = std::clamp(fftData[fftIndex] * 4.0f, 0.0f, 1.0f); // Gain up

            // Smooth
            m_smoothedValues[i] = m_smoothedValues[i] * 0.8f + value * 0.2f;

            // Mirror: Apply this value to both sides of the circle
            int mirrorIndex = m_barCount - 1 - i;
            m_smoothedValues[mirrorIndex] = m_smoothedValues[i];
        }
    }

    // Update Geometry
    float angleStep = (2.0f * M_PI) / m_barCount;

    for (int i = 0; i < m_barCount; ++i)
    {
        float angle = i * angleStep; // Radians

        // Position: Start at center + radius offset
        float x = m_center.x + std::cos(angle) * m_baseRadius;
        float y = m_center.y + std::sin(angle) * m_baseRadius;

        m_bars[i].setPosition({x, y});

        // Height: Base value + audio kick
        float h = m_smoothedValues[i] * 100.0f; // Max height 100
        if (h < 2.0f)
            h = 2.0f;

        m_bars[i].setSize({3.0f, h}); // Grow outward

        // Color: Make it "Amoeba" like (Green/Blue/Pink)
        sf::Color c(
            (std::uint8_t)(std::sin(angle + h * 0.01f) * 127 + 128),
            200,
            (std::uint8_t)(std::cos(angle) * 127 + 128),
            220);
        m_bars[i].setFillColor(c);

        // Ensure rotation points outward (degrees)
        m_bars[i].setRotation(sf::degrees(i * (360.0f / m_barCount) + 90.0f));
    }
}

void CircleVisualiser::draw(sf::RenderWindow &window)
{
    for (const auto &bar : m_bars)
    {
        window.draw(bar);
    }
}