#include "Transcriber.h"
#include "whisper.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <vector>

Transcriber* Transcriber::sInstance = nullptr;

Transcriber* Transcriber::getInstance()
{
    if (sInstance) {
        return sInstance;
    }
    sInstance = new Transcriber();
    return sInstance;
}

Transcriber::~Transcriber() {}


#include <sndfile.h>
#include <vector>
#include <string>

bool Transcriber::whisper_pcmf32_from_wav(const std::string& path, std::vector<float>& outPCM)
{
    SF_INFO sfinfo;
    SNDFILE* sndfile = sf_open(path.c_str(), SFM_READ, &sfinfo);
    if (!sndfile) {
        fprintf(stderr, "Failed to open '%s'\n", path.c_str());
        return false;
    }

    if (sfinfo.channels != 1 || sfinfo.samplerate != 16000) {
        fprintf(stderr, "Audio must be mono, 16 kHz. Got %d ch, %d Hz\n",
                sfinfo.channels, sfinfo.samplerate);
        sf_close(sndfile);
        return false;
    }

    outPCM.resize(sfinfo.frames);
    sf_readf_float(sndfile, outPCM.data(), sfinfo.frames);
    sf_close(sndfile);
    return true;
}

std::string Transcriber::transcribeAudio(const std::string& wavFile, const std::string& whisperModelPath, const std::string& language)
{
    
    struct whisper_context_params cparams = whisper_context_default_params();
    struct whisper_context* ctx = whisper_init_from_file_with_params(
            whisperModelPath.c_str(),
            cparams);

    if (!ctx) {
        std::cerr << "Failed to load Whisper model.\n";
        return "";
    }

    // Load WAV file
    std::vector<float> pcmf32;
    if (!whisper_pcmf32_from_wav(wavFile.c_str(), pcmf32)) {
        std::cerr << "Failed to read WAV file\n";
        whisper_free(ctx);
        return "";
    }

    whisper_full_params params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    params.print_progress = false;
    params.print_special = false;
    params.print_realtime = false;
    params.language = language.c_str();

    if (whisper_full(ctx, params, pcmf32.data(), pcmf32.size()) != 0) {
        std::cerr << "Failed to run Whisper inference\n";
        whisper_free(ctx);
        return "";
    }

    // Get the transcription
    std::string result;
    int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        result += whisper_full_get_segment_text(ctx, i);
        result += " ";
    }

    whisper_free(ctx);
    return result;
}


