#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/WindowHandle.hpp>
#include <Windows.h>
#include <iostream>
#include <optional>
#include <vector>

// Include our new visualizer
#include "visualizer/bar_visualizer.hpp"

using namespace std;

// ... [Keep your existing makeWindowTransparent function] ...
void makeWindowTransparent(sf::RenderWindow &window)
{
    HWND hwnd = static_cast<HWND>(window.getNativeHandle());
    LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, style | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
}

// ... [Keep your existing setAlwaysOnTop function] ...
void setAlwaysOnTop(sf::RenderWindow &window)
{
    HWND hwnd = static_cast<HWND>(window.getNativeHandle());
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

int main()
{
    constexpr unsigned int WINDOW_WIDTH = 800;
    constexpr unsigned int WINDOW_HEIGHT = 200;

    sf::RenderWindow window(
        sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}),
        "SoundWave",
        sf::Style::None);

    window.setFramerateLimit(60);

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    window.setPosition({(int)(desktop.size.x - WINDOW_WIDTH) / 2,
                        (int)(desktop.size.y - WINDOW_HEIGHT - 60)});

    makeWindowTransparent(window);
    setAlwaysOnTop(window);

    // === INIT VISUALIZER ===
    // 64 bars, fitting the window size
    BarVisualizer visualizer(64, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);

    // UI Setup
    sf::RectangleShape border(sf::Vector2f(WINDOW_WIDTH - 4.f, WINDOW_HEIGHT - 4.f));
    border.setPosition({2.f, 2.f});
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color(0, 255, 255, 50)); // Lower opacity border
    border.setOutlineThickness(2.f);

    bool isDragging = false;
    sf::Vector2i dragOffset;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
                window.close();

            if (const auto *key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
            }

            if (const auto *mouse = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouse->button == sf::Mouse::Button::Left)
                {
                    isDragging = true;
                    dragOffset = sf::Mouse::getPosition(window);
                }
            }

            if (const auto *mouse = event->getIf<sf::Event::MouseButtonReleased>())
            {
                if (mouse->button == sf::Mouse::Button::Left)
                    isDragging = false;
            }
        }

        if (isDragging)
        {
            window.setPosition(sf::Mouse::getPosition() - dragOffset);
        }

        // === UPDATE VISUALIZER ===
        // Pass empty vector to trigger "Demo Mode" (fake waves)
        visualizer.update({});

        // === RENDER ===
        window.clear(sf::Color(255, 0, 255)); // Magenta key color

        // Draw visualizer bars
        visualizer.draw(window);

        window.draw(border);
        window.display();
    }

    return 0;
}