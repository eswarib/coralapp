#include <fstream>
#include <iostream>

void redirectOutput(const std::string& logFile = "voice_typing.log") {
    static std::ofstream out(logFile, std::ios::app);
    std::cout.rdbuf(out.rdbuf());   // Redirect std::cout to file
    std::cerr.rdbuf(out.rdbuf());   // Redirect std::cerr to file
}

