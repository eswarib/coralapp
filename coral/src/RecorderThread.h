#pragma once
#ifndef RECORDER_THREAD_H
#define RECORDER_THREAD_H

#include <thread>
#include <atomic>
#include <memory>
#include <string>
#include "Config.h"
#include "concurrentQueue.h"
#include "AudioEvent.h"
#include "KeyDetector.h"

class RecorderThread
{
public:
	RecorderThread(const Config& config,
				   ConcurrentQueue<std::shared_ptr<AudioEvent>>& audioQueue,
				   KeyDetector& keyDetector);
	~RecorderThread();
	void start();
	void stop();
private:
	void run();
	std::thread mThread;
	std::atomic<bool> mRunning;
	const Config& mConfig;
	ConcurrentQueue<std::shared_ptr<AudioEvent>>& mAudioQueue;
	KeyDetector& mKeyDetector;
	// Push-to-talk toggle state (simple two-state machine: Idle <-> Recording).
	// Note: If more states (e.g., Paused, Error, Cancelling) are introduced,
	// consider extracting a small FSM class to manage lifecycle transitions.
	bool mIsRecording{false};
	std::string mActiveTriggerKey; // which key started recording (trigger or cmd)
	bool mPrevTriggerPressed{false};
	bool mPrevCmdPressed{false};
};

#endif // RECORDER_THREAD_H

