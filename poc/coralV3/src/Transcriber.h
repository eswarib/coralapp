#ifndef TRANSCRIBER_H
#define TRANSCRIBER_H
#include <string>
#include <vector>

class Transcriber
{
public:
    std::string transcribeAudio(const std::string& wavFile, const std::string& whisperModelPath, const std::string& language = "en");
    static Transcriber* getInstance();
    virtual ~Transcriber();
private:
    static Transcriber* sInstance;
    Transcriber() = default;
    Transcriber(const Transcriber&) = delete;
    Transcriber& operator=(const Transcriber&) = delete;
    bool whisper_pcmf32_from_wav(const std::string& path, std::vector<float>& outPCM);
    
};

#endif
