#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class BarVisualizer
{
public:
    BarVisualizer(int barCount, float width, float height);

    // Update bars with FFT data (empty = demo mode)
    void update(const std::vector<float> &fftData);

    void draw(sf::RenderWindow &window);
    void setSize(float width, float height);

private:
    int m_barCount;
    float m_width;
    float m_height;

    std::vector<sf::RectangleShape> m_bars;
    std::vector<float> m_smoothedValues; // For smooth animation

    void setupBars();
};