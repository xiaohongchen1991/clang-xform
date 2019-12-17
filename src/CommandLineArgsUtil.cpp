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

#include "CommandLineArgsUtil.hpp"

#include <algorithm>
#include <cstring>

std::vector<std::string> StripMatcherArgs(int& argc, const char* const * argv) {
  std::vector<std::string> strippedArgs;

  if (argc == 0) {
    return strippedArgs;
  }

  auto argSep = std::find_if(argv, argv + argc,
                             [](auto str){return (strstr(str, "--matcher-args") != nullptr);});
  if (argSep == argv + argc) {
    return strippedArgs;
  }

  strippedArgs.reserve((argv + argc) - argSep);
  strippedArgs.insert(strippedArgs.end(), argSep, argv + argc);

  argc = argSep - argv;

  return strippedArgs;
}

std::vector<std::vector<std::string> > GetMatcherArgs(const std::vector<std::string>& args,
                                                      const std::string& id) {
  std::vector<std::vector<std::string> > strippedArgs;
  if (args.empty()) {
    return strippedArgs;
  }

  auto begin = args.begin();
  while (begin != args.end()) {
    begin = std::find_if(begin, args.end(),
                         [&id](const std::string& str){return str.find("--matcher-args-" + id) != std::string::npos;});
    if (begin != args.end()) {
      auto end = std::find_if(begin + 1, args.end(),
                              [](const std::string& str){return str.find("--matcher-args-") != std::string::npos;});
      // the first arg will de discarded in cxxopt
      strippedArgs.emplace_back(begin, end);
      // update begin iterator
      begin = end;
    }
  }

  return strippedArgs;
}
