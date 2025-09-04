#include "Utils.h"
#include <cstdlib> // For getenv

std::string Utils::expandTilde(const std::string& path)
{
    if (path.empty() || path[0] != '~')
    {
        return path;
    }

    const char* home = getenv("HOME");
    if (home)
    {
        return std::string(home) + path.substr(1);
    }
    else
    {
        // If HOME is not set, return path as-is, though this is unlikely
        return path;
    }
}
