#include "RecorderThread.h"
#include "Recorder.h"
#include "Logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <memory>
#include <filesystem>

namespace
{
std::string generateUniqueFilename()
{
    // Get cross-platform temp directory
    std::filesystem::path baseTmp;
    try
    {
        baseTmp = std::filesystem::temp_directory_path();
    }
    catch (...)
    {
        baseTmp = std::filesystem::path("/tmp");
    }
    
    // Create coral subfolder in temp
    std::filesystem::path coralTmpDir = baseTmp / "coral";
    try
    { 
        std::filesystem::create_directories(coralTmpDir); 
    }
    catch (...)
    {
    }
    
    // Generate unique filename with timestamp
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::ostringstream oss;
    oss << "input_" << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S") << "_" << now_ms.count() << ".wav";
    
    // Return full path
    return (coralTmpDir / oss.str()).string();
}
}

RecorderThread::RecorderThread(const Config& config,
                               ConcurrentQueue<std::shared_ptr<AudioEvent>>& audioQueue,
                               KeyDetector& keyDetector)
    : mConfig(config), mAudioQueue(audioQueue), mKeyDetector(keyDetector), mRunning(false)
{
}

RecorderThread::~RecorderThread()
{
    stop();
}

void RecorderThread::start()
{
    mRunning = true;
    mThread = std::thread(&RecorderThread::run, this);
}

void RecorderThread::stop()
{
    mRunning = false;
    if (mThread.joinable())
    {
        mThread.join();
    }
}

void RecorderThread::run()
{
    INFO("RecorderThread started, waiting for trigger keys (push-to-talk toggle)...");

    // Simple toggle implementation using rising-edge detection.
    // States: Idle (not recording) and Recording. Transition on key press events.
    while (mRunning)
    {
        // Read current pressed state for both keys
        bool triggerPressed = mKeyDetector.isTriggerKeyPressed(mConfig.getTriggerKey());
        bool cmdPressed = mKeyDetector.isTriggerKeyPressed(mConfig.getCmdTriggerKey());

        // Rising edges
        bool triggerRising = triggerPressed && !mPrevTriggerPressed;
        bool cmdRising = cmdPressed && !mPrevCmdPressed;

        std::string risingKey;
        if (triggerRising)
        {
            DEBUG(3, "triggerKey rising edge detected");
            risingKey = mConfig.getTriggerKey();
        }
        else if (cmdRising)
        {
            DEBUG(3, "cmdTriggerKey rising edge detected");
            risingKey = mConfig.getCmdTriggerKey();
        }

        if (!risingKey.empty())
        {
            if (!mIsRecording)
            {
                // Start recording
                std::cout << "TRIGGER_DOWN" << std::endl;
                DEBUG(3, "Starting recording (toggle ON)");
                Recorder::getInstance()->startRecording();
                mIsRecording = true;
                mActiveTriggerKey = risingKey;
            }
            else
            {
                // Stop recording and enqueue
                std::cout << "TRIGGER_UP" << std::endl;
                std::string audioFile = generateUniqueFilename();
                Recorder::getInstance()->stopRecording(audioFile);
                DEBUG(3, "Finished recording (toggle OFF): " + audioFile);
                DEBUG(3, "Inserting audio file into queue: " + audioFile + ", triggered by: " + mActiveTriggerKey);
                mAudioQueue.push(std::make_shared<AudioEvent>(audioFile, mActiveTriggerKey));
                DEBUG(3, "Queue size after push: " + std::to_string(mAudioQueue.size()));
                mIsRecording = false;
                mActiveTriggerKey.clear();
            }
        }

        // Save previous states for edge detection and throttle loop
        mPrevTriggerPressed = triggerPressed;
        mPrevCmdPressed = cmdPressed;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

