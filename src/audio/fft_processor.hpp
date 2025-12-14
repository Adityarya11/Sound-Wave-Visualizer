#pragma once
#include <vector>
#include <complex>
#include "kissfft/kiss_fft.h"

class FftProcessor
{
public:
    FftProcessor(int sampleSize = 1024);
    ~FftProcessor();

    void calculate(const std::vector<float> &audioData, std::vector<float> &outputBars);

private:
    int N;
    kiss_fft_cfg cfg;
    kiss_fft_cpx *in;
    kiss_fft_cpx *out;
    std::vector<float> window;

    void createHanningWindow();
};