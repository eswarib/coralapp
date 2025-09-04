#include "Logger.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdarg>
#include <algorithm>
#include <vector>
#include <iomanip>
#include <ctime>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    logFile_.open(logFilePath_, std::ios_base::app);
    if (!logFile_.is_open()) {
        std::cerr << "Failed to open log file: " << logFilePath_ << std::endl;
    }
}

Logger::~Logger() {
    if (logFile_.is_open()) {
        logFile_.close();
    }
}

void Logger::setLogLevel(int debugLevel) {
    if (debugLevel >= 0 && debugLevel <= 5) {
        currentLevel_ = debugLevel;
    }
}

void Logger::log(LogLevel level, const std::string& msg) {
    if (static_cast<int>(level) <= currentLevel_) {
        logInternal(level, msg);
    }
}

void Logger::logf(LogLevel level, const char* fmt, ...) {
    if (static_cast<int>(level) > currentLevel_) {
        return;
    }
    std::vector<char> buffer(1024);
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(buffer.data(), buffer.size(), fmt, args);
    va_end(args);
    if (needed >= static_cast<int>(buffer.size())) {
        buffer.resize(needed + 1);
        va_start(args, fmt);
        vsnprintf(buffer.data(), buffer.size(), fmt, args);
        va_end(args);
    }
    logInternal(level, std::string(buffer.data()));
}

void Logger::setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (logFile_.is_open()) {
        logFile_.close();
    }
    logFilePath_ = filename;
    logFile_.open(logFilePath_, std::ios_base::app);
    if (!logFile_.is_open()) {
        std::cerr << "Failed to open log file: " << logFilePath_ << std::endl;
    }
}

void Logger::setLogToConsole(bool enable) {
    std::lock_guard<std::mutex> lock(mtx_);
    logToConsole_ = enable;
}

void Logger::logInternal(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx_);
    checkRotate();
    std::ostringstream oss;
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    oss << "[" << buf << "] [" << levelToString(level) << "] " << msg;
    if (msg.empty() || msg.back() != '\n') {
        oss << std::endl;
    }
    std::string out = oss.str();
    if (logFile_.is_open()) {
        logFile_ << out;
        logFile_.flush();
    }
    if (logToConsole_) {
        std::cout << out;
        std::cout.flush();
    }
}

void Logger::checkRotate() {
    if (logFile_.is_open()) {
        if (logFile_.tellp() > static_cast<std::streampos>(MAX_LOG_SIZE)) {
            logFile_.close();
            for (int i = MAX_LOG_BACKUPS - 1; i > 0; --i) {
                std::string oldPath = logFilePath_ + "." + std::to_string(i);
                std::string newPath = logFilePath_ + "." + std::to_string(i + 1);
                std::rename(oldPath.c_str(), newPath.c_str());
            }
            std::rename(logFilePath_.c_str(), (logFilePath_ + ".1").c_str());
            logFile_.open(logFilePath_, std::ios_base::app);
        }
    }
}

std::string Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::WARN: return "WARN";
        case LogLevel::INFO: return "INFO";
        case LogLevel::DEBUG1: return "DEBUG1";
        case LogLevel::DEBUG2: return "DEBUG2";
        case LogLevel::DEBUG3: return "DEBUG3";
        default: return "UNKNOWN";
    }
}

// ... rest of the original code ... 