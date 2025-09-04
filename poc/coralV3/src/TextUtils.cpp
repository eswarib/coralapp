#include "TextUtils.h"
#include <algorithm>
#include <cctype>

std::string TextUtils::trim(const std::string& s) {
    auto start = std::find_if_not(s.begin(), s.end(), ::isspace);
    auto end = std::find_if_not(s.rbegin(), s.rend(), ::isspace).base();
    return (start < end) ? std::string(start, end) : std::string();
}

std::string TextUtils::removeSpecialChars(const std::string& s, const std::unordered_set<char>& charsToRemove) {
    std::string result;
    for (char c : s) {
        if (charsToRemove.find(c) == charsToRemove.end()) {
            result += c;
        }
    }
    return result;
}

void TextUtils::lowercaseFirstNonSpace(std::string& s) {
    for (char& c : s) {
        if (!std::isspace(c)) {
            c = std::tolower(c);
            break;
        }
    }
}

void TextUtils::toLower(std::string& s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
} 