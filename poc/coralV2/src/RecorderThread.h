#pragma once
#ifndef RECORDER_THREAD_H
#define RECORDER_THREAD_H

#include <thread>
#include <atomic>
#include <memory>
#include "Config.h"
#include "concurrentQueue.h"
#include "AudioEvent.h"
#include "KeyDetector.h"

class RecorderThread {
public:
	RecorderThread(const Config& config,
				   ConcurrentQueue<std::shared_ptr<AudioEvent>>& audioQueue,
				   KeyDetector& keyDetector);
	~RecorderThread();
	void start();
	void stop();
private:
	void run();
	std::thread thread_;
	std::atomic<bool> running_;
	const Config& config_;
	ConcurrentQueue<std::shared_ptr<AudioEvent>>& audioQueue_;
	KeyDetector& keyDetector_;
};

#endif // RECORDER_THREAD_H

