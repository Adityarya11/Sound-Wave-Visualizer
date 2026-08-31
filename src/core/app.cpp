#include "core/app.hpp"

#include <algorithm>
#include <cmath>

#include <windows.h>

#include "core/aurora.hpp"

namespace
{
// How long the audio must stay quiet before the overlay fades away.
constexpr float kSilenceGrace = 1.5f;

// Frame rate while faded out. The window still has to present, but there is
// nothing to see, so there is no reason to burn a GPU frame every 16 ms.
constexpr unsigned int kIdleFps = 15;
constexpr unsigned int kActiveFps = 60;

bool modifiersHeld()
{
    const bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    return ctrl && alt;
}
} // namespace

bool App::init()
{
    m_config = Config::load();
    m_fft.setGain(m_config.gain);

    m_shaderOk = false;

    const unsigned int scale = m_config.scale;
    sf::Vector2u size = m_config.mode == Mode::Bars ? m_bars.preferredSize(scale)
                                                    : m_circle.preferredSize(scale);

    sf::Vector2i position{m_config.x, m_config.y};
    if (m_config.x == INT32_MIN || m_config.y == INT32_MIN)
    {
        // First run: lower centre of the primary monitor, clear of the taskbar.
        const int screenW = GetSystemMetrics(SM_CXSCREEN);
        const int screenH = GetSystemMetrics(SM_CYSCREEN);
        position = {(screenW - static_cast<int>(size.x)) / 2,
                    screenH - static_cast<int>(size.y) - 140};
    }

    if (!m_overlay.create(size, position))
        return false;

    // The shader needs a live GL context, so this has to happen after create().
    m_shaderOk = m_circle.load(paths::shaderFile());
    if (!m_shaderOk && m_config.mode == Mode::Circle)
        m_config.mode = Mode::Bars;

    applyMode(m_config.mode, true);

    // Audio failing is not fatal: the overlay stays up, silent and faded, and
    // poll() keeps retrying until a loopback endpoint appears.
    m_audio.start();

    return true;
}

VisualizerBase &App::active()
{
    if (m_config.mode == Mode::Bars || !m_shaderOk)
        return m_bars;
    return m_circle;
}

void App::applyMode(Mode mode, bool resizeWindow)
{
    if (mode == Mode::Circle && !m_shaderOk)
        mode = Mode::Bars;

    m_config.mode = mode;

    if (resizeWindow)
        m_overlay.resizeAboutCentre(active().preferredSize(m_config.scale));

    active().resize(sf::Vector2f(m_overlay.window().getSize()));
}

void App::setMoveMode(bool enabled)
{
    m_moveMode = enabled;
    m_dragging = false;
    m_overlay.setClickThrough(!enabled);
}

void App::nudgeScale(int steps)
{
    const int next = static_cast<int>(m_config.scale) + steps * 24;
    const auto clamped = static_cast<unsigned int>(std::clamp(next, 120, 1400));
    if (clamped == m_config.scale)
        return;

    m_config.scale = clamped;
    applyMode(m_config.mode, true);
}

void App::pollHotkeys()
{
    // Polled rather than registered: the window is WS_EX_NOACTIVATE and never
    // takes focus, so it receives no keyboard messages of its own.
    const bool mods = modifiersHeld();

    const auto edge = [&](Hotkey &hotkey) {
        const bool down = mods && (GetAsyncKeyState(hotkey.key) & 0x8000) != 0;
        const bool fired = down && !hotkey.wasDown;
        hotkey.wasDown = down;
        return fired;
    };

    if (edge(m_keyQuit))
        m_running = false;

    if (edge(m_keyMove))
        setMoveMode(!m_moveMode);

    if (edge(m_keyCycle))
        applyMode(m_config.mode == Mode::Circle ? Mode::Bars : Mode::Circle, true);

    if (edge(m_keyBigger))
        nudgeScale(+2);

    if (edge(m_keySmaller))
        nudgeScale(-2);
}

void App::pollEvents()
{
    sf::RenderWindow &window = m_overlay.window();

    while (const std::optional event = window.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            m_running = false;
            continue;
        }

        // Everything below is move-mode only. While locked the window is
        // click-through and these never arrive anyway.
        if (!m_moveMode)
            continue;

        if (const auto *key = event->getIf<sf::Event::KeyPressed>())
        {
            if (key->code == sf::Keyboard::Key::Escape)
                setMoveMode(false);
        }
        else if (const auto *pressed = event->getIf<sf::Event::MouseButtonPressed>())
        {
            if (pressed->button == sf::Mouse::Button::Left)
            {
                m_dragging = true;
                m_dragOffset = sf::Mouse::getPosition() - window.getPosition();
            }
            else if (pressed->button == sf::Mouse::Button::Right)
            {
                setMoveMode(false);
            }
        }
        else if (const auto *released = event->getIf<sf::Event::MouseButtonReleased>())
        {
            if (released->button == sf::Mouse::Button::Left)
                m_dragging = false;
        }
        else if (const auto *wheel = event->getIf<sf::Event::MouseWheelScrolled>())
        {
            nudgeScale(wheel->delta > 0.0f ? 1 : -1);
        }
    }

    if (m_dragging)
    {
        window.setPosition(sf::Mouse::getPosition() - m_dragOffset);
        m_overlay.clampToDesktop();
    }
}

void App::updateFade(float dt)
{
    const bool audible = m_fft.silenceSeconds() < kSilenceGrace;
    const bool visible = m_moveMode || !m_config.fadeWhenSilent || audible;

    const float target = visible ? m_config.opacity : 0.0f;

    // Snap in on the first note, drift out slowly so it never flickers
    // between tracks or during a quiet passage.
    const float tau = target > m_fade ? 0.10f : 0.70f;
    m_fade += (target - m_fade) * (1.0f - std::exp(-dt / tau));

    // A layered window never calls display(), so SFML's own frame limiter is
    // not in play; run() paces the loop using this flag instead.
    m_idleThrottled = m_fade < 0.01f && !m_moveMode;
}

void App::drawMoveChrome()
{
    sf::RenderWindow &window = m_overlay.window();
    const sf::Vector2f size(window.getSize());

    // Breathing outline so it is obvious the overlay is unlocked and will
    // swallow clicks until you lock it again.
    const float pulse = 0.65f + 0.35f * std::sin(m_time * 3.5f);

    // Without this wash the window is only draggable where the orb happens to
    // be painting: a layered window lets clicks through every zero-alpha pixel.
    sf::RectangleShape wash(size);
    wash.setFillColor(sf::Color(40, 12, 70, 46));
    window.draw(wash);

    sf::RectangleShape frame({size.x - 4.0f, size.y - 4.0f});
    frame.setPosition({2.0f, 2.0f});
    frame.setFillColor(sf::Color::Transparent);
    frame.setOutlineThickness(-2.0f);
    frame.setOutlineColor(aurora::color(0.28f, 0.85f * pulse));
    window.draw(frame);

    const float handle = 10.0f;
    const sf::Vector2f corners[4] = {
        {0.0f, 0.0f},
        {size.x - handle, 0.0f},
        {0.0f, size.y - handle},
        {size.x - handle, size.y - handle},
    };

    for (const sf::Vector2f &corner : corners)
    {
        sf::RectangleShape dot({handle, handle});
        dot.setPosition(corner);
        dot.setFillColor(aurora::color(0.55f, 0.9f * pulse));
        window.draw(dot);
    }
}

void App::render(float dt)
{
    sf::RenderWindow &window = m_overlay.window();

    VisualState state;
    state.bands = &m_fft.bands();
    state.level = m_fft.level();
    state.beat = m_fft.beat();
    state.time = m_time;
    state.opacity = m_fade;
    state.hue = m_hue;

    // Fully transparent clear. DWM composites whatever alpha we leave behind,
    // which is why nothing here uses a colour key any more.
    window.clear(sf::Color::Transparent);

    if (m_fade > 0.004f)
    {
        active().update(state, dt);
        active().draw(window);
    }

    if (m_moveMode)
        drawMoveChrome();

    m_overlay.present();
}

void App::persist()
{
    const sf::Vector2i position = m_overlay.window().getPosition();
    m_config.x = position.x;
    m_config.y = position.y;
    m_config.gain = m_fft.gain();
    m_config.save();
}

int App::run()
{
    sf::Clock clock;

    while (m_running && m_overlay.window().isOpen())
    {
        const float dt = std::clamp(clock.restart().asSeconds(), 1.0f / 240.0f, 0.25f);
        m_time += dt;

        pollHotkeys();
        pollEvents();

        m_audio.poll(dt);

        if (m_audio.readLatest(m_samples, 2048))
            m_fft.process(m_samples, m_audio.sampleRate(), dt);
        else
            m_fft.decay(dt);

        // Hue drifts continuously and jumps a little on every beat, so the
        // palette never sits still but also never strobes.
        m_hue = std::fmod(m_hue + dt * (0.012f + 0.05f * m_fft.beat()), 1.0f);

        updateFade(dt);
        m_overlay.keepOnTop(dt);
        render(dt);

        const float budget = 1.0f / static_cast<float>(m_idleThrottled ? kIdleFps : kActiveFps);
        const float spent = clock.getElapsedTime().asSeconds();
        if (spent < budget)
            sf::sleep(sf::seconds(budget - spent));
    }

    persist();
    m_audio.stop();
    m_overlay.window().close();
    return 0;
}
