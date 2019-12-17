/*
  MIT License

  Copyright (c) 2019 Xiaohong Chen

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef CORE_UTIL_HPP
#define CORE_UTIL_HPP

#include <string>
#include <vector>

// convert a macro into a string
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// forward declarations
namespace clang {
namespace tooling {

class CompilationDatabase;

} // end namespace toolong
} // end namespace clang

// execute the given command line
int ExecCmd(const std::string& cmd, std::string& result);
inline int ExecCmd(const std::string& cmd) {
  std::string tmp;
  return ExecCmd(cmd, tmp);
}

// parse config file and return string values for a given key
// return true if succeed
std::vector<std::string> ParseConfigFile(const std::string& fileName, const std::string& key);

// retrieve all matcher arguments from the given config file
std::vector<std::string> ParseConfigFileForMatcherArgs(const std::string& fileName);

int ProcessFiles(const clang::tooling::CompilationDatabase& compilationDatabase,
                 const std::vector<std::string>& inputFiles,
                 const std::string& outputFile,
                 const std::vector<std::string>& matchers,
                 const std::vector<std::string>& matcherArgs,
                 unsigned int numThreads);

#endif
