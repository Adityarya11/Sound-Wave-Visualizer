#include "core/overlay.hpp"

#include <SFML/OpenGL.hpp>

#include <algorithm>

#include <windows.h>

#ifndef GL_BGRA
#define GL_BGRA 0x80E1
#endif

namespace
{
constexpr float kTopmostInterval = 2.0f;

HWND handleOf(sf::RenderWindow &window)
{
    return static_cast<HWND>(window.getNativeHandle());
}
} // namespace

Overlay::~Overlay()
{
    destroySurface();
}

bool Overlay::create(sf::Vector2u size, sf::Vector2i position)
{
    m_window.create(sf::VideoMode(size), "Visualizer", sf::Style::None);
    m_window.setPosition(position);
    m_window.setView(sf::View(sf::FloatRect({0.0f, 0.0f}, sf::Vector2f(size))));

    // No setFramerateLimit: SFML implements it inside display(), which a
    // layered window never calls. App::run paces frames itself.

    applyExtendedStyle();

    if (!createSurface(size))
        return false;

    clampToDesktop();
    return m_window.isOpen();
}

bool Overlay::createSurface(sf::Vector2u size)
{
    destroySurface();

    if (size.x == 0 || size.y == 0)
        return false;

    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = static_cast<LONG>(size.x);
    // Positive height means bottom-up rows, which is the order glReadPixels
    // already returns. Matching them avoids flipping every frame.
    info.bmiHeader.biHeight = static_cast<LONG>(size.y);
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    HDC screen = GetDC(nullptr);
    if (screen == nullptr)
        return false;

    HDC memory = CreateCompatibleDC(screen);
    if (memory == nullptr)
    {
        ReleaseDC(nullptr, screen);
        return false;
    }

    void *pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(screen, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (bitmap == nullptr || pixels == nullptr)
    {
        DeleteDC(memory);
        ReleaseDC(nullptr, screen);
        return false;
    }

    m_screenDc = screen;
    m_memoryDc = memory;
    m_bitmap = bitmap;
    m_previousBitmap = SelectObject(memory, bitmap);
    m_pixels = pixels;
    m_surfaceSize = size;
    return true;
}

void Overlay::destroySurface()
{
    if (m_memoryDc != nullptr && m_previousBitmap != nullptr)
        SelectObject(static_cast<HDC>(m_memoryDc), static_cast<HGDIOBJ>(m_previousBitmap));

    if (m_bitmap != nullptr)
        DeleteObject(static_cast<HBITMAP>(m_bitmap));

    if (m_memoryDc != nullptr)
        DeleteDC(static_cast<HDC>(m_memoryDc));

    if (m_screenDc != nullptr)
        ReleaseDC(nullptr, static_cast<HDC>(m_screenDc));

    m_screenDc = nullptr;
    m_memoryDc = nullptr;
    m_bitmap = nullptr;
    m_previousBitmap = nullptr;
    m_pixels = nullptr;
    m_surfaceSize = {0, 0};
}

void Overlay::present()
{
    if (m_pixels == nullptr)
        return;

    const sf::Vector2u size = m_window.getSize();
    if (size != m_surfaceSize && !createSurface(size))
        return;

    // SFML draws into the back buffer; nothing has swapped it, so this is the
    // frame we just rendered. Alpha comes back already premultiplied, because
    // SFML's alpha blend multiplies colour by alpha on the way in and the
    // clear left the buffer at zero.
    glReadPixels(0, 0, static_cast<GLsizei>(m_surfaceSize.x), static_cast<GLsizei>(m_surfaceSize.y),
                 GL_BGRA, GL_UNSIGNED_BYTE, m_pixels);

    SIZE extent{static_cast<LONG>(m_surfaceSize.x), static_cast<LONG>(m_surfaceSize.y)};
    POINT source{0, 0};
    BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

    // pptDst stays null so UpdateLayeredWindow never fights setPosition while
    // the window is being dragged.
    UpdateLayeredWindow(handleOf(m_window), static_cast<HDC>(m_screenDc), nullptr, &extent,
                        static_cast<HDC>(m_memoryDc), &source, 0, &blend, ULW_ALPHA);
}

void Overlay::applyExtendedStyle()
{
    HWND hwnd = handleOf(m_window);
    if (hwnd == nullptr)
        return;

    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);

    // LAYERED is what makes UpdateLayeredWindow legal at all. TOOLWINDOW keeps
    // it out of Alt+Tab and the taskbar; NOACTIVATE means clicking it never
    // steals focus from whatever you are actually using.
    style |= WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST;

    // A layered window already passes clicks through wherever alpha is zero,
    // so this only needs to cover the pixels the orb actually paints.
    if (m_clickThrough)
        style |= WS_EX_TRANSPARENT;
    else
        style &= ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT);

    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, style);
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void Overlay::setClickThrough(bool enabled)
{
    if (m_clickThrough == enabled)
        return;

    m_clickThrough = enabled;
    applyExtendedStyle();
}

void Overlay::resizeAboutCentre(sf::Vector2u size)
{
    const sf::Vector2i oldPosition = m_window.getPosition();
    const sf::Vector2u oldSize = m_window.getSize();

    const sf::Vector2i centre{oldPosition.x + static_cast<int>(oldSize.x) / 2,
                              oldPosition.y + static_cast<int>(oldSize.y) / 2};

    m_window.setSize(size);
    m_window.setPosition({centre.x - static_cast<int>(size.x) / 2,
                          centre.y - static_cast<int>(size.y) / 2});
    m_window.setView(sf::View(sf::FloatRect({0.0f, 0.0f}, sf::Vector2f(size))));

    createSurface(size);
    applyExtendedStyle();
    clampToDesktop();
}

void Overlay::keepOnTop(float dt)
{
    m_topmostTimer -= dt;
    if (m_topmostTimer > 0.0f)
        return;

    m_topmostTimer = kTopmostInterval;

    HWND hwnd = handleOf(m_window);
    if (hwnd != nullptr)
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void Overlay::clampToDesktop()
{
    const int left = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int top = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
    if (width <= 0 || height <= 0)
        return;

    const sf::Vector2u size = m_window.getSize();
    sf::Vector2i position = m_window.getPosition();

    // Keep at least a corner reachable if a monitor was unplugged.
    constexpr int margin = 40;
    position.x = std::clamp(position.x, left - static_cast<int>(size.x) + margin,
                            left + width - margin);
    position.y = std::clamp(position.y, top - static_cast<int>(size.y) + margin,
                            top + height - margin);

    m_window.setPosition(position);
}
