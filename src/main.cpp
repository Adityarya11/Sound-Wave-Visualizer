#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/WindowHandle.hpp>
#include <Windows.h>
#include <iostream>
#include <optional>

// Includes
#include "audio/audio_capture.hpp"
#include "audio/fft_processor.hpp"
#include "visualizer/bar_visualizer.hpp"

// Define MiniAudio Here
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

using namespace std;

void makeWindowTransparent(sf::RenderWindow &window)
{
    HWND hwnd = static_cast<HWND>(window.getNativeHandle());
    SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
}

void setAlwaysOnTop(sf::RenderWindow &window)
{
    HWND hwnd = static_cast<HWND>(window.getNativeHandle());
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

int main()
{
    // 1. Init Audio
    AudioCapture audio;
    if (!audio.init())
    {
        cerr << "[ERROR] Failed to init audio!" << endl;
        return -1;
    }

    // 2. Init FFT & Visualizer
    FftProcessor fft(1024);
    BarVisualizer visualizer(64, 800.0f, 200.0f);
    vector<float> bars;

    // 3. Init Window
    sf::RenderWindow window(sf::VideoMode({800, 200}), "SoundWave", sf::Style::None);
    window.setFramerateLimit(60);

    auto desktop = sf::VideoMode::getDesktopMode();
    window.setPosition({(int)(desktop.size.x - 800) / 2, (int)(desktop.size.y - 200 - 50)});

    makeWindowTransparent(window);
    setAlwaysOnTop(window);

    // UI Border
    sf::RectangleShape border(sf::Vector2f(800 - 4.0f, 200 - 4.0f));
    border.setPosition({2.0f, 2.0f});
    border.setFillColor(sf::Color::Transparent);
    border.setOutlineColor(sf::Color(0, 255, 255, 100));
    border.setOutlineThickness(2.0f);

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
            window.setPosition(sf::Mouse::getPosition() - dragOffset);

        // --- Logic ---
        fft.calculate(audio.getAudioBuffer(), bars);
        visualizer.update(bars);

        // --- Render ---
        window.clear(sf::Color(255, 0, 255)); // Magenta Key

        visualizer.draw(window);
        window.draw(border);

        window.display();
    }
    return 0;
}