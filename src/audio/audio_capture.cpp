#include "audio_capture.hpp"

#include <algorithm>

namespace
{
// ~340 ms at 48 kHz. Large enough that a slow frame never loses audio, small
// enough that the data we read is always recent.
constexpr std::size_t kRingCapacity = 16384;

// How long to wait before retrying a device that failed to open.
constexpr float kRetryInterval = 1.5f;
} // namespace

AudioCapture::AudioCapture()
{
    m_ring.assign(kRingCapacity, 0.0f);
}

AudioCapture::~AudioCapture()
{
    stop();
}

void AudioCapture::dataCallback(ma_device *device, void *, const void *input, ma_uint32 frameCount)
{
    if (input == nullptr || frameCount == 0)
        return;

    auto *self = static_cast<AudioCapture *>(device->pUserData);
    const auto *samples = static_cast<const float *>(input);

    // Trust the device, not our request: WASAPI can hand back a different
    // channel count than we asked for, and de-interleaving by a wrong stride
    // turns music into noise.
    const ma_uint32 channels = device->capture.channels;
    if (channels == 0)
        return;

    std::lock_guard<std::mutex> lock(self->m_mutex);
    const std::size_t capacity = self->m_ring.size();

    for (ma_uint32 frame = 0; frame < frameCount; ++frame)
    {
        float sum = 0.0f;
        for (ma_uint32 c = 0; c < channels; ++c)
            sum += samples[frame * channels + c];

        self->m_ring[self->m_write] = sum / static_cast<float>(channels);
        self->m_write = (self->m_write + 1) % capacity;
    }

    self->m_hasData.store(true, std::memory_order_release);
}

void AudioCapture::notificationCallback(const ma_device_notification *notification)
{
    if (notification == nullptr || notification->pDevice == nullptr)
        return;

    if (notification->type == ma_device_notification_type_stopped)
    {
        auto *self = static_cast<AudioCapture *>(notification->pDevice->pUserData);
        if (self != nullptr)
            self->m_restartRequested.store(true, std::memory_order_release);
    }
}

bool AudioCapture::openDevice()
{
    ma_device_config config = ma_device_config_init(ma_device_type_loopback);
    config.capture.format = ma_format_f32;
    // channels = 0 and sampleRate = 0 mean "whatever the endpoint natively
    // uses". Forcing 44100 made miniaudio resample every buffer for nothing,
    // and made the FFT's bin-to-hertz mapping wrong on 48 kHz devices.
    config.capture.channels = 0;
    config.sampleRate = 0;
    config.dataCallback = dataCallback;
    config.notificationCallback = notificationCallback;
    config.pUserData = this;

    if (ma_device_init(&m_context, &config, &m_device) != MA_SUCCESS)
        return false;

    if (ma_device_start(&m_device) != MA_SUCCESS)
    {
        ma_device_uninit(&m_device);
        return false;
    }

    m_deviceReady = true;
    m_sampleRate = m_device.sampleRate != 0 ? m_device.sampleRate : 48000;
    m_restartRequested.store(false, std::memory_order_release);
    return true;
}

void AudioCapture::closeDevice()
{
    if (!m_deviceReady)
        return;

    ma_device_uninit(&m_device);
    m_deviceReady = false;
}

bool AudioCapture::start()
{
    if (!m_contextReady)
    {
        if (ma_context_init(nullptr, 0, nullptr, &m_context) != MA_SUCCESS)
            return false;
        m_contextReady = true;
    }

    return openDevice();
}

void AudioCapture::stop()
{
    // Guarded by the ready flags: the original destructor uninitialised a
    // device that init() had never successfully created.
    closeDevice();

    if (m_contextReady)
    {
        ma_context_uninit(&m_context);
        m_contextReady = false;
    }
}

void AudioCapture::poll(float dt)
{
    const bool stalled = m_deviceReady && !ma_device_is_started(&m_device);
    const bool wantsRestart = m_restartRequested.load(std::memory_order_acquire);

    if (!m_deviceReady || stalled || wantsRestart)
    {
        m_retryTimer -= dt;
        if (m_retryTimer > 0.0f)
            return;

        m_retryTimer = kRetryInterval;
        closeDevice();
        start();
    }
    else
    {
        m_retryTimer = 0.0f;
    }
}

bool AudioCapture::readLatest(std::vector<float> &out, std::size_t count)
{
    if (!m_hasData.load(std::memory_order_acquire))
        return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    const std::size_t capacity = m_ring.size();
    count = std::min(count, capacity);
    out.resize(count);

    const std::size_t start = (m_write + capacity - count) % capacity;
    for (std::size_t i = 0; i < count; ++i)
        out[i] = m_ring[(start + i) % capacity];

    return true;
}
