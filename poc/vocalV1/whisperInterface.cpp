#include "whisperInterface.h"
#include <fstream>
#include <sstream>
#include <cstdlib>

std::string transcribeAudio(const std::string &wavFile) {
    std::string cmd = "~/gitcode/whisper.cpp/build/bin/whisper-cli -m ~/gitcode/whisper.cpp/models/ggml-base.en.bin -f " + wavFile + " -otxt > /dev/null";
    std::system(cmd.c_str());

    std::ifstream in(wavFile + ".txt");
    std::stringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

