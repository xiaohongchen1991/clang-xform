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

#include "CommandLineArgs.hpp"
#include "cxxopts.hpp"
#include "CoreUtil.hpp"
#include "MatcherFactory.hpp"
#include "MatchCallbackBase.hpp"

#include "llvm/Support/Path.h"

CommandLineArgs ProcessCommandLine(int argc, char**argv)
{
  CommandLineArgs args;

  cxxopts::Options options("clang-xform", "Clang tool for large-scale C++ code refactoring");

  options.add_options("Group")
      ("h, help", "produce help message")
      ("j, num-threads", "number of threads", cxxopts::value<int>())
      ("p, compile-commands", "compile commands", cxxopts::value<std::string>())
      ("o, output", "output file", cxxopts::value<std::string>())
      ("a, apply", "apply replacements", cxxopts::value<std::string>())
      ("f, input-files", "input files", cxxopts::value<std::vector<std::string> >())
      ("m, matchers", "matchers to apply", cxxopts::value<std::vector<std::string> >())
      ("c, config", "config file", cxxopts::value<std::string>())
      ("d, display", "display registered matchers", cxxopts::value<bool>())
      ("q, quiet", "silent output", cxxopts::value<bool>())
      ("v, version", "version number", cxxopts::value<bool>())
      ("l, log", "log file", cxxopts::value<std::string>());

  options.parse_positional({"input-files"});

  auto result = options.parse(argc, argv);

  if (result.count("num-threads")) {
    args.numThreads = result["num-threads"].as<int>();
  }

  if (result.count("compile-commands")) {
    args.compileCommands = result["compile-commands"].as<std::string>();
  }

  if (result.count("output")) {
    args.outputFile = result["output"].as<std::string>();
  }

  if (result.count("apply")) {
    args.replaceFile = result["apply"].as<std::string>();
  }

  if (result.count("input-files")) {
    args.inputFiles = result["input-files"].as<std::vector<std::string> >();
  }

  if (result.count("matchers")) {
    args.matchers = result["matchers"].as<std::vector<std::string> >();
  }

  if (result.count("config")) {
    args.configFile = result["config"].as<std::string>();
  }

  if (result.count("display")) {
    args.display = result["display"].as<bool>();
  }

  if (result.count("quiet")) {
    args.quiet = result["quiet"].as<bool>();
  }

  if (result.count("version")) {
    args.version = result["version"].as<bool>();
  }

  if (result.count("log")) {
    args.logFile = result["log"].as<std::string>();
  }

  if (result.count("help"))
  {
    std::cout << options.help({"Group"}) << std::endl;
    exit(0);
  }

  return args;
}

bool AdjustCommandLineArgs(CommandLineArgs& args) {
  // if cfg file is specified, read the flags and adjust args
  if (args.configFile.empty()) {
    return false;
  }
    
  std::vector<std::string> extraMatchers = ParseConfigFile(args.configFile, "matchers");
  std::vector<std::string> extraInputFiles = ParseConfigFile(args.configFile, "input-files");
  args.matchers.insert(args.matchers.end(),
                       std::make_move_iterator(extraMatchers.begin()),
                       std::make_move_iterator(extraMatchers.end()));
  args.inputFiles.insert(args.inputFiles.end(),
                         std::make_move_iterator(extraInputFiles.begin()),
                         std::make_move_iterator(extraInputFiles.end()));

  return true;
}

bool ValidateCommandLineArgs(const CommandLineArgs& args, bool hasClangFlags, std::string& errmsg) {
  // validate flags
  // Flags --compile-commands and -- [CLANG_FLAGS] are mutually exclusive
  if ((!args.compileCommands.empty() + hasClangFlags) > 1) {
    errmsg = "Options --compile-commands and -- [CLANG_FLAGS] are mutually exclusive!";
    return false;
  }
  int flagsum = !args.compileCommands.empty()
      + !args.inputFiles.empty()
      + !args.matchers.empty() + !args.outputFile.empty()
      + !args.replaceFile.empty()
      + args.display;
  // Flags --apply should be mutually exclusive with the rest options
  if (!args.replaceFile.empty() && flagsum > 1) {
    errmsg = "Options --apply should be mutually exclusive with the rest options";
    return false;
  }
  // Flags --display should be mutually exclusive with the rest options
  if (args.display && flagsum > 1) {
    errmsg = "Options --display should be mutually exclusive with the rest options";
    return false;
  }
  // option --output should be a file with yaml extension
  if (!args.outputFile.empty() && llvm::sys::path::extension(args.outputFile) != ".yaml") {
    errmsg = "Output file extension is not yaml";
    return false;
  }
  // option --apply should be a file with yaml extension
  if (!args.replaceFile.empty() && llvm::sys::path::extension(args.replaceFile) != ".yaml") {
    errmsg = "Replacement file extension is not yaml";
    return false;
  }
  // arguments for option --matchers should be registered
  MatcherFactory& factory = MatcherFactory::Instance();
  for (const auto& matcher : args.matchers) {
    if (factory.getMatcherMap().find(matcher) == factory.getMatcherMap().end()) {
      // matcher id does not exist
      errmsg = "Matcher ID: " + matcher + " is not registered!";
      return false;
    }
  }

  return true;
}
