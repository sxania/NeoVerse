// Utils.h - small helper functions shared by the whole project
#pragma once
#include <string>
#include <vector>

namespace util {

std::string timestamp();                                   // "YYYY-MM-DD HH:MM:SS"
std::string trim(const std::string& s);                    // strip leading/trailing whitespace
std::string sanitize(const std::string& s);                // remove '|' and line breaks (file safety)
std::string csvEscape(const std::string& s);               // wrap in quotes for CSV export
// Split on 'delim'. If maxParts > 0 the last part receives the remainder of the string.
std::vector<std::string> split(const std::string& s, char delim, std::size_t maxParts = 0);

}  // namespace util
