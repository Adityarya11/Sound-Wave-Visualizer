#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/WindowHandle.hpp>
#include <Windows.h>
#include <dwmapi.h>
#include <iostream>
#include <optional>
#include <vector>

// Define MiniAudio
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "audio/audio_capture.hpp"
#include "audio/fft_processor.hpp"
#include "visualizer/bar_visualizer.hpp"

using namespace std;

// === THE GLASS FIX ===
void makeWindowTransparent(sf::RenderWindow &window)
{
    HWND hwnd = static_cast<HWND>(window.getNativeHandle());

    // 1. Set the window to be a "Layered" window (Required for transparency)
    SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_LAYERED);

    // 2. The "Glass" Magic
    // This tells Windows: "Render the whole window as a glass surface"
    // The alpha channel (transparency) will be respected by the OS.
    MARGINS margins = {-1};
    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

void setAlwaysOnTop(sf::RenderWindow &window)
{
    SetWindowPos(static_cast<HWND>(window.getNativeHandle()),
                 HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

int main()
{
    constexpr unsigned int WINDOW_WIDTH = 800;
    constexpr unsigned int WINDOW_HEIGHT = 200;
    constexpr int NUM_BARS = 64;

    // Create the Window
    sf::RenderWindow window(
        sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}),
        "SoundWave",
        sf::Style::None // Borderless
    );
    window.setFramerateLimit(60);

    // Position it
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    window.setPosition({(int)(desktop.size.x - WINDOW_WIDTH) / 2,
                        (int)(desktop.size.y - WINDOW_HEIGHT - 60)});

    // Apply Transparency
    makeWindowTransparent(window);
    setAlwaysOnTop(window);

    // Setup Audio & Visuals
    AudioCapture audioCapture;
    audioCapture.init();

    FftProcessor fftProcessor(1024);
    std::vector<float> fftOutput;
    BarVisualizer visualizer(NUM_BARS, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);

    // Optional Background (Toggle 'B')
    sf::RectangleShape background(sf::Vector2f((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT));
    background.setFillColor(sf::Color(0, 0, 0, 150)); // Semi-transparent black

    bool isDragging = false;
    sf::Vector2i dragOffset;
    bool showBackground = false;

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
                if (key->code == sf::Keyboard::Key::B)
                    showBackground = !showBackground;
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

        // Logic
        std::vector<float> audioBuffer = audioCapture.getAudioBuffer();
        if (!audioBuffer.empty())
            fftProcessor.calculate(audioBuffer, fftOutput);
        visualizer.update(fftOutput);

        // === RENDER ===
        // IMPORTANT: Clear with specific Transparent color
        window.clear(sf::Color::Transparent);

        if (showBackground)
            window.draw(background);

        visualizer.draw(window);
        window.display();
    }
    return 0;
}