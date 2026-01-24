#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class CircleVisualiser
{
public:
    CircleVisualiser(int barCount, float baseRadius, sf::Vector2f centrePosition);

    void draw(sf::RenderWindow &window);
    void update(const std::vector<float> &fftdata);

    void setCenter(sf::Vector2f center);

private:
    int m_barCount;
    float m_baseRadius;
    sf::Vector2f m_center;

    std::vector<sf::RectangleShape> m_bars;
    std::vector<float> m_smoothedValues;

    void setupBars();
};
