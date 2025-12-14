#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class BarVisualizer
{
public:
    BarVisualizer(int barCount, float width, float height);

    void update(const std::vector<float> &frequencyData);
    void draw(sf::RenderWindow &window);

private:
    int m_barCount;
    float m_width;
    float m_height;
    std::vector<float> m_smoothHeights;
    std::vector<sf::RectangleShape> m_bars;
};