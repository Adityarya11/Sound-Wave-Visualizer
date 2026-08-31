#include "fft_processor.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace
{
constexpr float kPi = 3.14159265358979f;

// Window of dB that maps onto a full-height bar. Anything quieter than the
// floor reads as nothing; the ceiling sits below 0 dBFS because a single FFT
// bin never carries the whole signal.
constexpr float kDbFloor = -80.0f;
constexpr float kDbCeil = -12.0f;

constexpr float kSilenceDb = -62.0f;

float smoothTowards(float current, float target, float dt, float tau)
{
    const float k = 1.0f - std::exp(-dt / tau);
    return current + (target - current) * k;
}
} // namespace

FftProcessor::FftProcessor(int fftSize, int bandCount)
    : m_size(fftSize), m_bandCount(bandCount)
{
    m_cfg = kiss_fft_alloc(m_size, 0, nullptr, nullptr);
    m_in.resize(m_size);
    m_out.resize(m_size);

    m_window.resize(m_size);
    for (int i = 0; i < m_size; ++i)
        m_window[i] = 0.5f * (1.0f - std::cos(2.0f * kPi * i / (m_size - 1)));

    m_bands.assign(m_bandCount, 0.0f);
    m_raw.assign(m_bandCount, 0.0f);
    m_bandStart.assign(m_bandCount, 1);
    m_bandEnd.assign(m_bandCount, 2);
}

FftProcessor::~FftProcessor()
{
    std::free(m_cfg);
}

void FftProcessor::rebuildBandEdges(unsigned int sampleRate)
{
    if (sampleRate == 0 || sampleRate == m_edgeRate)
        return;

    m_edgeRate = sampleRate;

    const float binHz = static_cast<float>(sampleRate) / static_cast<float>(m_size);
    const int maxBin = m_size / 2 - 1;

    const float fLow = 28.0f;
    const float fHigh = std::min(16000.0f, static_cast<float>(sampleRate) * 0.46f);
    const float ratio = fHigh / fLow;

    m_bassBands = 0;
    for (int b = 0; b < m_bandCount; ++b)
    {
        const float t0 = static_cast<float>(b) / m_bandCount;
        const float t1 = static_cast<float>(b + 1) / m_bandCount;
        const float f0 = fLow * std::pow(ratio, t0);
        const float f1 = fLow * std::pow(ratio, t1);

        // Bin 0 is DC. Always skip it: it is pure offset and would peg the
        // first band permanently.
        const int k0 = std::clamp(static_cast<int>(std::floor(f0 / binHz)), 1, maxBin);
        const int k1 = std::clamp(static_cast<int>(std::ceil(f1 / binHz)), k0 + 1, maxBin + 1);

        m_bandStart[b] = k0;
        m_bandEnd[b] = k1;

        if (f1 <= 160.0f)
            ++m_bassBands;
    }

    m_bassBands = std::max(m_bassBands, 1);
}

void FftProcessor::decay(float dt)
{
    dt = std::clamp(dt, 1.0f / 240.0f, 0.25f);

    for (float &band : m_bands)
        band = smoothTowards(band, 0.0f, dt, 0.18f);

    m_level = smoothTowards(m_level, 0.0f, dt, 0.20f);
    m_beat *= std::exp(-dt / 0.16f);
    m_silence += dt;
}

void FftProcessor::process(const std::vector<float> &samples, unsigned int sampleRate, float dt)
{
    dt = std::clamp(dt, 1.0f / 240.0f, 0.25f);

    if (static_cast<int>(samples.size()) < m_size)
    {
        decay(dt);
        return;
    }

    rebuildBandEdges(sampleRate);

    const std::size_t offset = samples.size() - static_cast<std::size_t>(m_size);

    double energy = 0.0;
    for (int i = 0; i < m_size; ++i)
    {
        const float s = samples[offset + i];
        energy += static_cast<double>(s) * s;
        m_in[i].r = s * m_window[i];
        m_in[i].i = 0.0f;
    }

    const float rms = static_cast<float>(std::sqrt(energy / m_size));
    const float rmsDb = 20.0f * std::log10(rms + 1e-7f);

    kiss_fft(m_cfg, m_in.data(), m_out.data());

    // 2/N for the one-sided spectrum, divided by the Hann window coherent
    // gain of 0.5, so a full-scale tone lands at roughly 0 dB.
    const float norm = 4.0f / static_cast<float>(m_size);
    const float dbSpan = kDbCeil - kDbFloor;

    float frameMax = 0.0f;

    for (int b = 0; b < m_bandCount; ++b)
    {
        float peak = 0.0f;
        for (int k = m_bandStart[b]; k < m_bandEnd[b]; ++k)
        {
            const float re = m_out[k].r;
            const float im = m_out[k].i;
            peak = std::max(peak, std::sqrt(re * re + im * im) * norm);
        }

        const float db = 20.0f * std::log10(peak + 1e-7f);
        float v = std::clamp((db - kDbFloor) / dbSpan, 0.0f, 1.0f);

        // Music loses roughly 6 dB per octave going up, so without a tilt the
        // treble half of the display barely moves.
        const float t = static_cast<float>(b) / std::max(1, m_bandCount - 1);
        v *= 0.55f + 0.75f * t;

        m_raw[b] = v;
        frameMax = std::max(frameMax, v);
    }

    // Slow auto-gain so a quiet track still fills the orb, without pumping.
    m_peak = std::max(frameMax, smoothTowards(m_peak, 0.18f, dt, 2.5f));
    const float agc = std::clamp(0.90f / std::max(m_peak, 0.25f), 0.8f, 2.5f);

    // Fast attack, slow release: transients snap, tails glide.
    const float attack = 1.0f - std::exp(-dt / 0.020f);
    const float release = 1.0f - std::exp(-dt / 0.180f);

    for (int b = 0; b < m_bandCount; ++b)
    {
        const float target = std::clamp(m_raw[b] * agc * m_gain, 0.0f, 1.0f);
        const float k = target > m_bands[b] ? attack : release;
        m_bands[b] += (target - m_bands[b]) * k;
    }

    m_level = smoothTowards(m_level, std::clamp((rmsDb + 60.0f) / 45.0f, 0.0f, 1.0f), dt, 0.12f);
    m_silence = rmsDb < kSilenceDb ? m_silence + dt : 0.0f;

    // Beat = bass rising meaningfully above its own recent average.
    float bass = 0.0f;
    for (int b = 0; b < m_bassBands; ++b)
        bass += m_bands[b];
    bass /= static_cast<float>(m_bassBands);

    const float flux = bass - m_bassAverage * 1.30f;
    m_bassAverage = smoothTowards(m_bassAverage, bass, dt, 0.35f);

    const float hit = std::clamp(flux * 3.5f, 0.0f, 1.0f);
    m_beat = std::max(m_beat * std::exp(-dt / 0.16f), hit);
}
