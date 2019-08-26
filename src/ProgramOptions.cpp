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

#include "ProgramOptions.hpp"
#include "cxxopts.hpp"

#include <cstring>

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

std::vector<const char*> StripMatcherArgs(int& argc, char** argv) {
  std::vector<const char*> strippedArgs;

  if (argc == 0) {
    return strippedArgs;
  }

  char** argSep = std::find_if(argv, argv + argc,
                               [](auto str){return (strstr(str, "--matcher-args") != nullptr);});
  if (argSep == argv + argc) {
    return strippedArgs;
  }

  strippedArgs.reserve((argv + argc) - argSep);
  strippedArgs.insert(strippedArgs.end(), argSep, argv + argc);

  argc = argSep - argv;

  return strippedArgs;
}
