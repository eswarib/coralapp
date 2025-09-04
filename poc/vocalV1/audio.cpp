#include "audio.h"
#include <portaudio.h>
#include <sndfile.h>
#include <iostream>
#include <vector>

#define SAMPLE_RATE 16000
#define FRAMES_PER_BUFFER 512
#define NUM_SECONDS 5
#define NUM_CHANNELS 1

static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo*,
                          PaStreamCallbackFlags,
                          void *userData) {
    std::vector<float> *recordedSamples = (std::vector<float>*)userData;
    const float *in = (const float*)inputBuffer;
    if (in != nullptr) {
        recordedSamples->insert(recordedSamples->end(), in, in + framesPerBuffer);
    }
    return paContinue;
}

void recordAudio(const std::string &filename) {
    Pa_Initialize();
    std::vector<float> recordedSamples;

    PaStream *stream;
    Pa_OpenDefaultStream(&stream, NUM_CHANNELS, 0, paFloat32,
                         SAMPLE_RATE, FRAMES_PER_BUFFER, recordCallback, &recordedSamples);
    Pa_StartStream(stream);
    Pa_Sleep(NUM_SECONDS * 1000);
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    SF_INFO sfinfo;
    sfinfo.channels = NUM_CHANNELS;
    sfinfo.samplerate = SAMPLE_RATE;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE *outfile = sf_open(filename.c_str(), SFM_WRITE, &sfinfo);
    if (!outfile) {
        std::cerr << "Error opening audio file for writing.\n";
        return;
    }

    std::vector<short> intData(recordedSamples.size());
    for (size_t i = 0; i < recordedSamples.size(); ++i) {
        intData[i] = static_cast<short>(recordedSamples[i] * 32767);
    }

    sf_write_short(outfile, intData.data(), intData.size());
    sf_close(outfile);
}

