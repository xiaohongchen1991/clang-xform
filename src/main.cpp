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

#include "clang_xform_config.hpp"
#include "CommandLineArgs.hpp"
#include "CommandLineArgsUtil.hpp"
#include "cxxlog.hpp"
#include "CoreUtil.hpp"
#include "MatcherFactory.hpp"
#include "MatchCallbackBase.hpp"
#include "ApplyReplacements.hpp"
#include "cxxopts.hpp"
#include "CodeXformException.hpp"

#include <iostream>
#include <tuple>
#include <sstream>
#include <iterator>
#include <cassert>

#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/JSONCompilationDatabase.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

using namespace cxxlog;
using namespace clang::tooling;
using namespace clang;
using namespace llvm::sys;
using namespace llvm;

int main(int argc, char **argv) {
  std::string errMsg;
  // first read and remove flags after seperator --
  std::unique_ptr<CompilationDatabase> compilations = FixedCompilationDatabase::loadFromCommandLine(argc, argv, errMsg);
  // strip off arguments for matchers
  std::vector<const char*> matcherArgs = StripMatcherArgs(argc, argv);
  // parse command line options
  CommandLineArgs args;
  try {
    args = ProcessCommandLine(argc, argv);
  }
  catch (cxxopts::OptionParseException& e) {
    std::cerr << e.what() << '\n';
    std::cerr << "See sbcodexform --help" << '\n';
    exit(1);
  }

  // if cfg file is specified, read the flags and adjust args
  try {
    AdjustCommandLineArgs(args);
  } catch(FileSystemException& e) {
    std::cerr << e.what() << '\n';
    exit(1);
  }

  // validate flags
  if (!ValidateCommandLineArgs(args, compilations != nullptr, errMsg)) {
    std::cerr << errMsg << '\n';
    std::cerr << "See sbcodexform --help" << '\n';
    return 1;
  }

  // read flags
  std::string compileCommands = std::move(args.compileCommands);
  std::string configFile = std::move(args.configFile);
  std::vector<std::string> inputFiles = std::move(args.inputFiles);
  std::vector<std::string> matchers = std::move(args.matchers);
  unsigned int numThreads = args.numThreads;
  std::string outputFile = std::move(args.outputFile);
  std::string replaceFile = std::move(args.replaceFile);
  std::string logFile = std::move(args.logFile);
  bool display = args.display;
  bool quiet = args.quiet;
  bool version = args.version;

  // setup log file
  if (logFile.empty()) {
    logFile = "clang-xform.log";
  }
  RegisterLogFile log_file(logFile);
  if (quiet) {
    TrivialLog::Verbosity() = verbosity::quiet;
  }

  // is --output is not set, by default the replacements will be applied at the end of the program.
  SmallString<256> tmp_path;
  std::string outputFileName = "tmp_output_file.yaml";
  if (outputFile.empty() && replaceFile.empty()) {
    outputFile = outputFileName;
  }

  // create a new file for output
  if (fs::exists(outputFile)) {
    fs::remove(outputFile);
  }
  fs::current_path(tmp_path);
  fs::createUniqueFile(outputFile, tmp_path);

  // convert dirs and files to absolute path
  if (!compileCommands.empty()) {
    tmp_path = compileCommands;
    fs::make_absolute(tmp_path);
    compileCommands = tmp_path.str().str();
  }
  if (!outputFile.empty()) {
    tmp_path = outputFile;
    fs::make_absolute(tmp_path);
    outputFile = tmp_path.str().str();
  }
  if (!replaceFile.empty()) {
    tmp_path = replaceFile;
    fs::make_absolute(tmp_path);
    replaceFile = tmp_path.str().str();
  }
  if (!logFile.empty()) {
    tmp_path = logFile;
    fs::make_absolute(tmp_path);
    logFile = tmp_path.str().str();
  }
  for(auto& file : inputFiles) {
    tmp_path = file;
    fs::make_absolute(tmp_path);
    file = tmp_path.str().str();
  }

  // print out version number
  if (version) {
    const char* appname = strrchr(argv[0], '/');
    appname = appname ? (appname + 1) : argv[0];
    std::cout << appname << " version "
              << CLANG_XFORM_VERSION_MAJOR << '.'
              << CLANG_XFORM_VERSION_MINOR << '.'
              << CLANG_XFORM_VERSION_PATCH << '\n';
    return 0;
  }
  // display registered matchers
  if (display) {
    MatcherFactory& factory = MatcherFactory::Instance();
    const auto& matcherMap = factory.getMatcherMap();
    for (const auto& pair : matcherMap) {
      std::cout << pair.first << '\n';
    }
    return 0;
  }
  // when -a is given
  if (!replaceFile.empty()) {
    // apply replacements
    TRIVIAL_LOG(info) << "Apply replacements: " << replaceFile << '\n';
    try {
      ApplyReplacements(replaceFile);
    }
    catch (ConflictedReplacementsException& e) {
      // swallow this exception
      std::cerr << e.what() << '\n';
    }
    catch (CodeXformException& e) {
      std::cerr << e.what() << '\n';
      exit(1);
    }
    return 0;
  }


  // store cwd
  std::string cwd;
  fs::current_path(tmp_path);
  cwd = tmp_path.str();

  int status = 0;
  // components option is not specified
  // if -p is given
  if (!compileCommands.empty())
  {
    TRIVIAL_LOG(info) << "Loading file: " << compileCommands << '\n';
    auto pos = compileCommands.find_last_of('/');
    fs::set_current_path(compileCommands.substr(0, pos));
    // use jsonCompilationDatabase provided in json file
    compilations =
        JSONCompilationDatabase::loadFromFile(compileCommands.substr(pos + 1,
                                                                     compileCommands.length() -pos - 1),
                                              errMsg,
                                              JSONCommandLineSyntax::Gnu);
    if (!compilations) {
      std::cerr << "Error while trying to load a json compilation database:\n"
                << errMsg << '\n'
                << "Json file does not exist.\n";
      return 1;
    }
    if (inputFiles.empty())
    {
      inputFiles = compilations->getAllFiles();
    }
    try {
      status = ProcessFiles(*compilations, inputFiles, outputFile, matchers, matcherArgs, numThreads);
    }
    catch(RunClangToolException& e) {
      std::cerr << e.what() << '\n';
      exit(1);
    }
  }
  else
  {
    if (!inputFiles.empty()) {
      if (compilations) {
        // use fixedCompilationDatabase provided in command line
        try {
          status = ProcessFiles(*compilations, inputFiles, outputFile, matchers, matcherArgs, numThreads);
        }
        catch(RunClangToolException& e) {
          std::cerr << e.what() << '\n';
          exit(1);
        }
      }
      else {
        // auto detect compile_commands.json file if only on input file
        compilations =  CompilationDatabase::autoDetectFromSource(inputFiles[0],
                                                                  errMsg);
        if (!compilations) {
          std::cerr << "Error while trying to load a compilation database:\n"
                    << errMsg << "Running without flags.\n";
          return 1;
        }
        try {
          status = ProcessFiles(*compilations, inputFiles, outputFile, matchers, matcherArgs, numThreads);
        }
        catch(RunClangToolException& e) {
          std::cerr << e.what() << '\n';
          exit(1);
        }
      }
    }
    else {
      std::cerr << "No given input file!" << '\n';
      return 1;
    }
  }

  // restore cwd
  fs::set_current_path(cwd);

  // apply replacement automatically if the outputFile is default
  if (outputFile.rfind(outputFileName) != std::string::npos) {
    // apply replacements
    TRIVIAL_LOG(info) << "Apply replacements: " << outputFile << '\n';
    try {
      ApplyReplacements(outputFile);
    }
    catch (ConflictedReplacementsException& e) {
      // swallow this exception
      std::cerr << e.what() << '\n';
    }
    catch (CodeXformException& e) {
      std::cerr << e.what() << '\n';
      exit(1);
    }
  } else {
    std::cout << '\n' << "Replacements are stored in " << outputFile << "\n\n";
    std::cout << "To apply replacements, run:" << "\n\n";
    std::cout << "clang-xform -a " + outputFile << '\n';
  }

  std::cout << '\n' << "Check " << logFile << " to see log information" << "\n\n";

  return status;
}
