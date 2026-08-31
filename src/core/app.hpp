#pragma once

#include <vector>

#include "audio/audio_capture.hpp"
#include "audio/fft_processor.hpp"
#include "core/config.hpp"
#include "core/overlay.hpp"
#include "visualizer/bar_visualizer.hpp"
#include "visualizer/circle_visualizer.hpp"

class App
{
public:
    bool init();
    int run();

private:
    struct Hotkey
    {
        int key;
        bool wasDown = false;
    };

    VisualizerBase &active();
    void applyMode(Mode mode, bool resizeWindow);
    void pollHotkeys();
    void pollEvents();
    void updateFade(float dt);
    void render(float dt);
    void drawMoveChrome();
    void setMoveMode(bool enabled);
    void nudgeScale(int steps);
    void persist();

    Config m_config;
    Overlay m_overlay;
    AudioCapture m_audio;
    FftProcessor m_fft{2048, 64};
    CircleVisualizer m_circle;
    BarVisualizer m_bars;

    std::vector<float> m_samples;

    Hotkey m_keyMove{'V'};
    Hotkey m_keyCycle{'B'};
    Hotkey m_keyQuit{'Q'};
    Hotkey m_keyBigger{0x26};  // VK_UP
    Hotkey m_keySmaller{0x28}; // VK_DOWN

    float m_time = 0.0f;
    float m_hue = 0.0f;
    float m_fade = 0.0f;
    bool m_shaderOk = false;
    bool m_moveMode = false;
    bool m_dragging = false;
    bool m_idleThrottled = false;
    sf::Vector2i m_dragOffset;
    bool m_running = true;
};
