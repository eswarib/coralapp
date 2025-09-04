#pragma once
#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <iostream>
#include <sstream>
#include <ctime>

enum class LogLevel {
    ERROR = 0,
    WARN = 1,
    INFO = 2,
    DEBUG1 = 3,
    DEBUG2 = 4,
    DEBUG3 = 5
};

class Logger {
public:
    static Logger& getInstance();
    void setLogLevel(int debugLevel); // 0=ERROR, 1=WARN, 2=INFO, 3=DEBUG1, 4=DEBUG2, 5=DEBUG3
    void log(LogLevel level, const std::string& msg);
    void logf(LogLevel level, const char* fmt, ...);
    void setLogFile(const std::string& filename);
    void setLogToConsole(bool enable);
private:
    Logger();
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    int currentLevel_ = 2; // Default INFO
    std::ofstream logFile_;
    std::string logFilePath_ = "voiceOut.log";
    std::mutex mtx_;
    bool logToConsole_ = true;
    void logInternal(LogLevel level, const std::string& msg);
    std::string levelToString(LogLevel level);
    void checkRotate();
    static constexpr size_t MAX_LOG_SIZE = 10 * 1024 * 1024; // 10MB
    static constexpr int MAX_LOG_BACKUPS = 3;
};

#define ERROR(msg)   Logger::getInstance().log(LogLevel::ERROR, msg)
#define WARN(msg)    Logger::getInstance().log(LogLevel::WARN, msg)
#define INFO(msg)    Logger::getInstance().log(LogLevel::INFO, msg)
#define DEBUG(level, msg) Logger::getInstance().log(static_cast<LogLevel>(2 + (level)), msg) 