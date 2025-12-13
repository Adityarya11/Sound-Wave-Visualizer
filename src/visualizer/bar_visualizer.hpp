#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

class BarVisualizer
{
public:
    // Constructor: sets up the number of bars and window size
    BarVisualizer(int barCount, float width, float height);

    // Update the bars with new audio data (amplitudes: 0.0 to 1.0)
    // If audioData is empty, it will simulate a "demo mode"
    void update(const std::vector<float> &audioData);

    // Draw the bars to the window
    void draw(sf::RenderWindow &window);

    // Handle resizing if the window changes size
    void setSize(float width, float height);

private:
    int m_barCount;
    float m_width;
    float m_height;

    // The graphical bars
    std::vector<sf::RectangleShape> m_bars;

    // Helper to arranging bars
    void setupBars();
};