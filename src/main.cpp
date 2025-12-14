#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/WindowHandle.hpp>
#include <Windows.h>
#include <dwmapi.h>
#include <iostream>
#include <optional>
#include <vector>

// ==========================================
// MiniAudio Implementation
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
// ==========================================

#include "audio/audio_capture.hpp"
#include "audio/fft_processor.hpp"
#include "visualizer/bar_visualizer.hpp"

using namespace std;

// --- MODERN TRANSPARENCY (DWM) ---
void makeWindowTransparent(sf::RenderWindow &window)
{
    HWND hwnd = static_cast<HWND>(window.getNativeHandle());

    // 1. Force window to be a "Popup" (Standard for overlays)
    SetWindowLong(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);

    // 2. Extend the "Glass Frame" into the whole client area
    //    This tells Windows 10/11 to handle the alpha channel for us.
    MARGINS margins;
    margins.cxLeftWidth = -1; // -1 means "Extend to entire window"
    margins.cxRightWidth = -1;
    margins.cyTopHeight = -1;
    margins.cyBottomHeight = -1;

    DwmExtendFrameIntoClientArea(hwnd, &margins);
}

void setAlwaysOnTop(sf::RenderWindow &window)
{
    HWND hwnd = static_cast<HWND>(window.getNativeHandle());
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

int main()
{
    constexpr unsigned int WINDOW_WIDTH = 800;
    constexpr unsigned int WINDOW_HEIGHT = 200;
    constexpr int NUM_BARS = 64;

    // Create Window
    sf::RenderWindow window(
        sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}),
        "SoundWave",
        sf::Style::None);
    window.setFramerateLimit(60);

    // Position Window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    window.setPosition({(int)(desktop.size.x - WINDOW_WIDTH) / 2,
                        (int)(desktop.size.y - WINDOW_HEIGHT - 60)});

    // Apply The Fixes
    makeWindowTransparent(window);
    setAlwaysOnTop(window);

    // Audio Setup
    AudioCapture audioCapture;
    bool audioInitialized = audioCapture.init();
    if (!audioInitialized)
        cerr << "[ERROR] Audio Init Failed" << endl;

    // Visualizer Setup
    FftProcessor fftProcessor(1024);
    std::vector<float> fftOutput;
    BarVisualizer visualizer(NUM_BARS, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);

    // Background (Toggle with 'B')
    sf::RectangleShape background(sf::Vector2f((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT));
    background.setFillColor(sf::Color(10, 10, 20, 200)); // Dark Semi-Transparent

    bool isDragging = false;
    sf::Vector2i dragOffset;
    bool showBackground = false; // Start invisible

    cout << "[INFO] Running using DWM Transparency." << endl;
    cout << "[INFO] Press 'B' to toggle background box." << endl;

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
        if (!audioBuffer.empty() && audioInitialized)
        {
            fftProcessor.calculate(audioBuffer, fftOutput);
        }
        visualizer.update(fftOutput);

        // === RENDER ===
        // CRITICAL FIX: Clear with Transparent, NOT Magenta
        window.clear(sf::Color::Transparent);

        if (showBackground)
            window.draw(background);

        visualizer.draw(window);
        window.display();
    }

    return 0;
}