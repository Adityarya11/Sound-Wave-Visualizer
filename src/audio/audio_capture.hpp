#pragma once
#include <vector>
#include <mutex>
#include "miniaudio.h"

class AudioCapture
{
public:
    AudioCapture();
    ~AudioCapture();

    bool init();
    std::vector<float> getAudioBuffer(); // Returns a safe copy of data

private:
    ma_device device;
    ma_context context;
    std::vector<float> audioBuffer;
    std::mutex bufferMutex; // Prevents crashing when reading/writing at same time

    static void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount);
};