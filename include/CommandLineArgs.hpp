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

#ifndef COMMAND_LINE_ARGS_HPP
#define COMMAND_LINE_ARGS_HPP

#include <string>
#include <vector>
#include <limits>

// Command line arguments.
struct CommandLineArgs
{
  CommandLineArgs() = default;

  // list of files to parse (empty means all files)
  std::vector<std::string> inputFiles;
  // number of threads
  int numThreads = std::numeric_limits<int>::max();
  // compile commands
  std::string compileCommands;
  // output file
  std::string outputFile;
  // replacement file
  std::string replaceFile;
  // matchers to apply
  std::vector<std::string> matchers;
  // config file
  std::string configFile;
  // display registered matchers
  bool display = false;
  // silent output
  bool quiet = false;
  // version number
  bool version = false;
  // log file
  std::string logFile;
};

// Parse the command line arguments.
CommandLineArgs ProcessCommandLine(int argc, char**argv);

// update command line arguments
// return true if adjusted
bool AdjustCommandLineArgs(CommandLineArgs& args, std::vector<std::string>& matcherArgs);

// validate given command line arguments
// return false if invalid arguments exist and set error message
bool ValidateCommandLineArgs(const CommandLineArgs& args, const std::vector<std::string>& matcherArgs,
                             bool hasClangFlags, std::string& errmsg);

#endif // COMMAND_LINE_ARGS_HPP
