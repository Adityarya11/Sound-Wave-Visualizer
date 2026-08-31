#pragma once

#include <SFML/Graphics.hpp>

// A borderless, always-on-top, per-pixel-transparent window.
//
// Transparency is done the layered-window way: render with OpenGL, read the
// RGBA back, and hand it to UpdateLayeredWindow. Two tidier-looking options
// were tried first and both fail on current Windows 11 builds -
// DwmEnableBlurBehindWindow over an empty region and DwmExtendFrameIntoClientArea
// both return S_OK and then composite the window opaque black. The original
// magenta colour key does work, but can only ever produce hard binary edges,
// which is useless for a soft glow.
//
// The readback costs one glReadPixels per frame (~460 KB for a 340px orb),
// which is cheap enough at 60 fps and buys correct alpha everywhere.
class Overlay
{
public:
    ~Overlay();

    bool create(sf::Vector2u size, sf::Vector2i position);

    sf::RenderWindow &window() { return m_window; }
    const sf::RenderWindow &window() const { return m_window; }

    // Pushes the rendered frame to the screen. Replaces window.display():
    // a layered window never shows its GL surface, only what it is given here.
    void present();

    // Click-through means mouse input falls through to whatever is behind, so
    // the overlay never steals a click. Turned off only in move mode.
    void setClickThrough(bool enabled);
    bool clickThrough() const { return m_clickThrough; }

    // Resizes about the current centre so the orb does not jump.
    void resizeAboutCentre(sf::Vector2u size);

    // Other windows can push us down the z-order; re-assert periodically.
    void keepOnTop(float dt);

    void clampToDesktop();

private:
    void applyExtendedStyle();
    bool createSurface(sf::Vector2u size);
    void destroySurface();

    sf::RenderWindow m_window;

    // Win32 handles, kept as void* so this header stays free of windows.h.
    void *m_screenDc = nullptr;
    void *m_memoryDc = nullptr;
    void *m_bitmap = nullptr;
    void *m_previousBitmap = nullptr;
    void *m_pixels = nullptr;

    sf::Vector2u m_surfaceSize{0, 0};
    bool m_clickThrough = true;
    float m_topmostTimer = 0.0f;
};
