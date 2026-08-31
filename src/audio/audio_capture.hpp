#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <vector>

#include "miniaudio.h"

// WASAPI loopback capture of whatever Windows is currently playing.
//
// Audio arrives on miniaudio's realtime thread in bursts whose size we do not
// control, so samples land in a ring buffer and the render thread pulls the
// newest window out of it. Writing into a plain vector from index 0 (as the
// first version did) leaves stale tail samples whenever a callback delivers
// fewer frames than the FFT window, which smears the spectrum.
class AudioCapture
{
public:
    AudioCapture();
    ~AudioCapture();

    AudioCapture(const AudioCapture &) = delete;
    AudioCapture &operator=(const AudioCapture &) = delete;

    bool start();
    void stop();

    bool isRunning() const { return m_deviceReady; }
    unsigned int sampleRate() const { return m_sampleRate; }

    // Copies the newest `count` samples into `out`, oldest first.
    // False means the device has not produced a single frame yet.
    bool readLatest(std::vector<float> &out, std::size_t count);

    // Windows silently kills a loopback stream when the default output device
    // changes (headphones plugged in, Bluetooth connected). Call once a frame.
    void poll(float dt);

private:
    static void dataCallback(ma_device *device, void *output, const void *input, ma_uint32 frameCount);
    static void notificationCallback(const ma_device_notification *notification);

    bool openDevice();
    void closeDevice();

    ma_context m_context{};
    ma_device m_device{};
    bool m_contextReady = false;
    bool m_deviceReady = false;

    std::vector<float> m_ring;
    std::size_t m_write = 0;
    mutable std::mutex m_mutex;

    std::atomic<bool> m_hasData{false};
    std::atomic<bool> m_restartRequested{false};

    unsigned int m_sampleRate = 48000;
    float m_retryTimer = 0.0f;
};
