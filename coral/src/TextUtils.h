#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <string>
#include <unordered_set>
#include <vector>

class TextUtils
{
public:
    static std::string trim(const std::string& s);
    static std::string removeSpecialChars(const std::string& s, const std::unordered_set<char>& charsToRemove);
    static std::string removeSpecialSubstrings(const std::string& s, const std::vector<std::string>& substringsToRemove);
    static void lowercaseFirstNonSpace(std::string& s);
    static void toLower(std::string& s);
};

#endif // TEXT_UTILS_H