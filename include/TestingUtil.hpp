#ifndef TESTING_UTIL_HPP
#define TESTING_UTIL_HPP

#include <string>

bool CompareFiles(const std::string& p1, const std::string& p2);

bool InitTest(const std::string& dirPath,
              const std::string& outputFile);

bool IsEmptyFile(const std::string& file);

#endif
