#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/WindowHandle.hpp>
#include <Windows.h>
#include <dwmapi.h>
#include <iostream>
#include <optional>
#include <vector>

// ==========================================
// 🔴 CRITICAL FIX: MiniAudio Implementation
// This creates the actual code for the audio engine.
// Without this, you get "Unresolved External Symbol" errors.
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
// ==========================================

// Audio & Processing
#include "audio/audio_capture.hpp"
#include "audio/fft_processor.hpp"

// Visualizer
#include "visualizer/bar_visualizer.hpp"

using namespace std;

// --- Transparency Helper ---
void makeWindowTransparent(sf::RenderWindow &window)
{
    HWND hwnd = static_cast<HWND>(window.getNativeHandle());

    // 1. Set Layered Window Style (Required for transparency)
    LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, style | WS_EX_LAYERED);

    // 2. Set Magenta (255, 0, 255) as the "Key" color
    // Any pixel of this exact color becomes 100% transparent (invisible)
    SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);

    // 3. Optional: Add DWM Blur for smoother edges on Windows 10/11
    DWM_BLURBEHIND bb = {0};
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.fEnable = TRUE;
    bb.hRgnBlur = CreateRectRgn(0, 0, -1, -1);
    DwmEnableBlurBehindWindow(hwnd, &bb);
    DeleteObject(bb.hRgnBlur);
}

void setAlwaysOnTop(sf::RenderWindow &window)
{
    HWND hwnd = static_cast<HWND>(window.getNativeHandle());
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

// --- Main ---
int main()
{
    constexpr unsigned int WINDOW_WIDTH = 800;
    constexpr unsigned int WINDOW_HEIGHT = 200;
    constexpr int NUM_BARS = 64;

    // Create Window
    sf::RenderWindow window(
        sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}),
        "SoundWave",
        sf::Style::None); // Borderless
    window.setFramerateLimit(60);

    // Position Window
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    window.setPosition({(int)(desktop.size.x - WINDOW_WIDTH) / 2,
                        (int)(desktop.size.y - WINDOW_HEIGHT - 60)});

    // Apply Transparency Fixes
    makeWindowTransparent(window);
    setAlwaysOnTop(window);

    // Init Audio
    AudioCapture audioCapture;
    if (!audioCapture.init())
    {
        std::cerr << "[ERROR] Failed to init audio capture!" << std::endl;
    }
    else
    {
        std::cout << "[INFO] Audio capture ready." << std::endl;
    }

    // Init Processors
    FftProcessor fftProcessor(1024);
    std::vector<float> fftOutput;
    BarVisualizer visualizer(NUM_BARS, (float)WINDOW_WIDTH, (float)WINDOW_HEIGHT);

    // Background (Toggle with 'B')
    sf::RectangleShape background(sf::Vector2f((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT));
    background.setFillColor(sf::Color(15, 15, 25, 200)); // Semi-transparent dark

    bool isDragging = false;
    sf::Vector2i dragOffset;
    bool showBackground = true;

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

        // Audio Logic
        std::vector<float> audioBuffer = audioCapture.getAudioBuffer();
        if (!audioBuffer.empty())
            fftProcessor.calculate(audioBuffer, fftOutput);

        visualizer.update(fftOutput);

        // Render
        // 1. Clear with Magenta (The Key Color) -> This punches the hole in the window
        window.clear(sf::Color(255, 0, 255));

        // 2. Draw Background (Optional) -> This draws ON TOP of the transparent hole
        if (showBackground)
        {
            window.draw(background);
        }

        // 3. Draw Bars
        visualizer.draw(window);

        window.display();
    }

    return 0;
}