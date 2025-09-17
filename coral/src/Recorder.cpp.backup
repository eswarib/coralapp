#include "Recorder.h"
#include <portaudio.h>
#include <sndfile.h>
#include <iostream>
#include <vector>
#include "stderrSilencer.h"
#include "stdoutSilencer.h"

#define SAMPLE_RATE 16000
#define FRAMES_PER_BUFFER 512
#define NUM_CHANNELS 1

Recorder* Recorder::sInstance = nullptr;

Recorder* Recorder::getInstance()
{
	if (sInstance) {
		return sInstance;
	}
	sInstance = new Recorder();
	return sInstance;
}

Recorder::~Recorder() {
    if (stream_) {
        Pa_StopStream(stream_);
        Pa_CloseStream(stream_);
    }
    Pa_Terminate();
}

static int recordCallback(const void* inputBuffer, void* /*outputBuffer*/,
                         unsigned long framesPerBuffer,
                         const PaStreamCallbackTimeInfo*,
                         PaStreamCallbackFlags,
                         void* userData)
{
    std::vector<float>* recordedSamples = (std::vector<float>*)userData;
    const float* in = (const float*)inputBuffer;
    if (in != nullptr) {
        recordedSamples->insert(recordedSamples->end(), in, in + framesPerBuffer);
    }
    return paContinue;
}

void Recorder::startRecording()
{
    stderrSilencer silencer;
    stdoutSilencer stdoutSilencer;
    Pa_Initialize();
    recordedSamples_.clear();
    Pa_OpenDefaultStream(&stream_, 1, 0, paFloat32,
                         SAMPLE_RATE, FRAMES_PER_BUFFER, recordCallback, &recordedSamples_);
    Pa_StartStream(stream_);
}

void Recorder::stopRecording(const std::string& filename)
{
    if (!stream_) return;
    Pa_StopStream(stream_);
    Pa_CloseStream(stream_);
    stream_ = nullptr;
    Pa_Terminate();

    // Use the filename as provided by the caller (RecorderThread)
    std::string outFilename = filename;

    SF_INFO sfinfo;
    sfinfo.channels = 1;
    sfinfo.samplerate = SAMPLE_RATE;
    sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

    SNDFILE* outfile = sf_open(outFilename.c_str(), SFM_WRITE, &sfinfo);
    if (!outfile) {
        std::cerr << "Error opening audio file for writing.\n";
        return;
    }

    std::vector<short> intData(recordedSamples_.size());
    for (size_t i = 0; i < recordedSamples_.size(); ++i) {
        intData[i] = static_cast<short>(recordedSamples_[i] * 32767);
    }

    sf_write_short(outfile, intData.data(), intData.size());
    sf_close(outfile);
}

