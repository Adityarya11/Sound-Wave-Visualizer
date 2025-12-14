#include "fft_processor.hpp"
#include <cmath>
#include <algorithm>

FftProcessor::FftProcessor(int sampleSize) : N(sampleSize)
{
    cfg = kiss_fft_alloc(N, 0, NULL, NULL);
    in = new kiss_fft_cpx[N];
    out = new kiss_fft_cpx[N];
    createHanningWindow();
}

FftProcessor::~FftProcessor()
{
    free(cfg);
    delete[] in;
    delete[] out;
}

void FftProcessor::createHanningWindow()
{
    window.resize(N);
    for (int i = 0; i < N; ++i)
    {
        window[i] = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * i / (N - 1)));
    }
}

void FftProcessor::calculate(const std::vector<float> &audioData, std::vector<float> &outputBars)
{
    if (audioData.size() < N)
        return;

    for (int i = 0; i < N; ++i)
    {
        in[i].r = audioData[i] * window[i];
        in[i].i = 0;
    }

    kiss_fft(cfg, in, out);

    int usefulBins = N / 2;
    if (outputBars.size() != usefulBins)
        outputBars.resize(usefulBins);

    for (int i = 0; i < usefulBins; ++i)
    {
        float magnitude = std::sqrt(out[i].r * out[i].r + out[i].i * out[i].i);
        float db = 20.0f * std::log10(magnitude + 1.0f);
        outputBars[i] = db / 60.0f; // Normalize
    }
}