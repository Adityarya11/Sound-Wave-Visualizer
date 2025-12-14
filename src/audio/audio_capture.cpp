#include "audio_capture.hpp"
#include <iostream>

AudioCapture::AudioCapture()
{
    audioBuffer.resize(1024, 0.0f);
}

AudioCapture::~AudioCapture()
{
    ma_device_uninit(&device);
    ma_context_uninit(&context);
}

void AudioCapture::data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
    if (pInput == nullptr)
        return;

    AudioCapture *self = (AudioCapture *)pDevice->pUserData;
    const float *inputData = (const float *)pInput;

    // Lock memory while writing
    std::lock_guard<std::mutex> lock(self->bufferMutex);

    // Copy left channel only (stereo to mono)
    for (size_t i = 0; i < frameCount && i < self->audioBuffer.size(); i++)
    {
        self->audioBuffer[i] = inputData[i * 2];
    }
}

bool AudioCapture::init()
{
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS)
        return false;

    ma_device_config config = ma_device_config_init(ma_device_type_loopback);
    config.capture.format = ma_format_f32;
    config.capture.channels = 2;
    config.sampleRate = 44100;
    config.dataCallback = data_callback;
    config.pUserData = this;

    if (ma_device_init(&context, &config, &device) != MA_SUCCESS)
        return false;
    if (ma_device_start(&device) != MA_SUCCESS)
        return false;

    return true;
}

std::vector<float> AudioCapture::getAudioBuffer()
{
    std::lock_guard<std::mutex> lock(bufferMutex);
    return audioBuffer;
}