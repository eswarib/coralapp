#ifndef RECORDER_H
#define RECORDER_H

#include <string>
#include <portaudio.h>
#include <vector>

class Recorder
{
public:
	void startRecording();
	void stopRecording(const std::string &filename);
	static Recorder * getInstance();
	virtual ~Recorder();
private:
	Recorder() = default;
	Recorder(const Recorder&) = delete;
	Recorder& operator=(const Recorder&) = delete;

	static Recorder * sInstance;
	PaStream* stream_ = nullptr;
	std::vector<float> recordedSamples_;
};

#endif

