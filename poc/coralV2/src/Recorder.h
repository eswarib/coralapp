#ifndef RECORDER_H
#define RECORDER_H

#include <string>
#include <portaudio.h>

class Recorder
{
public:
	void recordAudio(const std::string &filename);
	static Recorder * getInstance();
	virtual ~Recorder();
private:
	Recorder() = default;
	Recorder(const Recorder&) = delete;
	Recorder& operator=(const Recorder&) = delete;

	static Recorder * sInstance;
};

#endif

