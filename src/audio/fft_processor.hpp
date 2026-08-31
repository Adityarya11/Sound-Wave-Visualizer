#pragma once

#include <vector>

#include "kissfft/kiss_fft.h"

// Turns a window of mono samples into something a visualiser can draw:
// perceptually spaced bands, an overall level, and a beat impulse.
//
// The important part is band mapping. Raw FFT bins are linearly spaced in
// hertz, so half of them cover 10-20 kHz where music has almost no energy.
// Feeding bins straight to bars leaves everything past the first few bars
// dead, which is exactly the "bars are all bunched on the left" symptom.
// Bands here are spaced logarithmically, the way pitch actually works.
class FftProcessor
{
public:
    explicit FftProcessor(int fftSize = 2048, int bandCount = 64);
    ~FftProcessor();

    FftProcessor(const FftProcessor &) = delete;
    FftProcessor &operator=(const FftProcessor &) = delete;

    // samples may be longer than the FFT size; the newest window is used.
    void process(const std::vector<float> &samples, unsigned int sampleRate, float dt);

    // Let everything fall to rest when no audio is arriving at all.
    void decay(float dt);

    const std::vector<float> &bands() const { return m_bands; }
    float level() const { return m_level; }
    float beat() const { return m_beat; }
    float silenceSeconds() const { return m_silence; }

    void setGain(float gain) { m_gain = gain; }
    float gain() const { return m_gain; }

private:
    void rebuildBandEdges(unsigned int sampleRate);

    int m_size;
    int m_bandCount;

    kiss_fft_cfg m_cfg = nullptr;
    std::vector<kiss_fft_cpx> m_in;
    std::vector<kiss_fft_cpx> m_out;
    std::vector<float> m_window;

    std::vector<float> m_bands;
    std::vector<float> m_raw;
    std::vector<int> m_bandStart;
    std::vector<int> m_bandEnd;

    unsigned int m_edgeRate = 0;
    int m_bassBands = 1;

    float m_level = 0.0f;
    float m_beat = 0.0f;
    float m_bassAverage = 0.0f;
    float m_peak = 0.35f;
    float m_silence = 0.0f;
    float m_gain = 1.0f;
};
