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

#include "ProgramOptions.hpp"
#include "Logger.hpp"
#include "MyFrontendAction.hpp"
#include "ToolingUtil.hpp"
#include "ApplyReplacements.hpp"
#include "MatcherFactory.hpp"
#include "cxxopts.hpp"

#include <iostream>
#include <numeric>
#include <algorithm>
#include <memory>
#include <thread>
#include <chrono>
#include <future>
#include <tuple>
#include <atomic>
#include <sstream>
#include <iterator>

#include <boost/assert.hpp>

#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/JSONCompilationDatabase.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

using namespace clang::tooling;
using namespace clang;
using namespace llvm::sys;
using namespace llvm;

int ProcessFiles(const CompilationDatabase& compilationDatabase,
                 const std::vector<std::string>& inputFiles,
                 const std::string& outputFile,
                 const std::vector<std::string>& matchers,
                 unsigned int numThreads)
{
    // We are trying to achieve a balance between two competing efficiency sources:
    // - Efficiency is gained by having the files processed by individual threads.  This is an
    // "embarrasing parallel problem."
    // - Efficiency is gained by having each clang::tooling::ClangTool do multiple files because
    // each tool only needs to
    //   create the AST of each header file once.
    //
    // To obtain a balance, we have each thread have one clang::toolingClangTool instance and each
    // instance do several files.
    auto const hwConcurrency = std::max(
        1u, std::min(numThreads, std::max(4u, std::thread::hardware_concurrency())));
    auto const numFiles = inputFiles.size();
    
    auto const filesPerCore = std::max(
        size_t(3), static_cast<size_t>(std::round(double(numFiles) / double(hwConcurrency))));

    numThreads = ceil(double(numFiles) / double(filesPerCore));

    // std::cout << "numFiles: " << numFiles << std::endl;
    // std::cout << "filesPerCore: " << filesPerCore << std::endl;
    // std::cout << "numThreads: " << numThreads << std::endl;
    
    std::vector<std::thread> threads;
    std::vector<std::future<std::tuple<int, std::string> > > futures;

    // loop through the list of files and process filesPerCore during each loop iteration.
    for (size_t beginRange = 0, endRange = std::min(numFiles, filesPerCore); beginRange < numFiles;
         beginRange = endRange, endRange = std::min(numFiles, endRange + filesPerCore)) {

        std::vector<std::string> fileSubset(inputFiles.begin() + beginRange,
                                            inputFiles.begin() + endRange);

        std::packaged_task<std::tuple<int, std::string>()> task(
            [&compilationDatabase, &outputFile, &matchers, files = std::move(fileSubset)]()
            {
                clang::tooling::ClangTool tool(compilationDatabase, files);

                std::stringstream diagnostics;
                llvm::raw_os_ostream raw_ostream(diagnostics);
                clang::DiagnosticOptions diagOpts;
                // replace DiagnosticLogger with TextDiagnosticPrinter to debug
                auto printDiagnostics =
                    std::make_unique<DiagnosticLogger>(raw_ostream, &diagOpts);

                tool.setDiagnosticConsumer(printDiagnostics.release());

                //tool.setDiagnosticConsumer(new clang::IgnoringDiagConsumer());

                const int toolStatus = tool.run(std::make_unique<MyFrontendActionFactory>(outputFile, matchers).get());

                return std::make_tuple(toolStatus, diagnostics.str());
            });
        futures.push_back(task.get_future());
        // let the current thread perform the last batch of files.
        if (endRange < numFiles) {
            // start new threads
            threads.emplace_back(std::move(task));
        }
        else {
            task();
        }
    }
    
    // wait for all the futures to finish and return the number of failed tasks.
    auto ret = std::accumulate(futures.begin(), futures.end(), 0,
                               [](int sum, std::future<std::tuple<int, std::string> >& f)
                               {
                                   auto next = f.get();
                                   const int nextStatus = std::get<0>(next);
                                   const std::string nextErrorMessage = std::move(std::get<1>(next));

                                   // when the tool fails, log any diagnostics accumulated from clang.
                                   if ((0 != nextStatus) && !nextErrorMessage.empty()) {
                                       TRIVIAL_LOG(info) << nextErrorMessage << '\n';
                                   }
                            
                                   return sum + nextStatus;
                               });

    // join threads
    std::for_each(threads.begin(), threads.end(),
                  [](std::thread& t)
                  {
                      if (t.joinable())
                      {
                          t.join();
                      }
                  });
    return ret;
}

int main(int argc, char **argv) {
        std::string errMsg;
    // first read and remove flags after seperator --
    std::unique_ptr<CompilationDatabase> compilations = FixedCompilationDatabase::loadFromCommandLine(argc, argv, errMsg);

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
    
    // if cfg file is specified, read the flags
    if (!configFile.empty()) {
        std::vector<std::string> extraMatchers;
        std::vector<std::string> extraInputFiles;
        if (!ParseConfigFile(configFile, "matchers", extraMatchers)) {
            std::cerr << "Failed in parsing config file: " << configFile << '\n';
            return 1;
        }
        if (!ParseConfigFile(configFile, "input-files", extraInputFiles)) {
            std::cerr << "Failed in parsing config file: " << configFile << '\n';
            return 1;
        }
        matchers.insert(matchers.end(),
                        std::make_move_iterator(extraMatchers.begin()),
                        std::make_move_iterator(extraMatchers.end()));
        inputFiles.insert(inputFiles.end(),
                        std::make_move_iterator(extraInputFiles.begin()),
                        std::make_move_iterator(extraInputFiles.end()));
    }

    // validate flags
    // Flags --compile-commands and -- [CLANG_FLAGS] are mutually exclusive
    if ((!compileCommands.empty() + (compilations != nullptr)) > 1) {
        std::cerr << "Options --compile-commands and -- [CLANG_FLAGS] are mutually exclusive!" << '\n';
        std::cerr << "See clang-xform --help" << '\n';
        return 1;
    }
    int flagsum = !compileCommands.empty() + !configFile.empty()
        + !inputFiles.empty() + !matchers.empty()
        + !outputFile.empty() + !replaceFile.empty()
        + display + version;
    // Flags --version should be mutually exclusive with the rest options
    if (version && flagsum > 1) {
        std::cerr << "Options --version should be mutually exclusive with the rest options" << '\n';
        std::cerr << "See clang-xform --help" << '\n';
        return 1;
    }
    // Flags --apply should be mutually exclusive with the rest options
    if (!replaceFile.empty() && flagsum > 1) {
        std::cerr << "Options --apply should be mutually exclusive with the rest options" << '\n';
        std::cerr << "See clang-xform --help" << '\n';
        return 1;
    }
    // Flags --display should be mutually exclusive with the rest options
    if (display && flagsum > 1) {
        std::cerr << "Options --display should be mutually exclusive with the rest options" << '\n';
        std::cerr << "See clang-xform --help" << '\n';
        return 1;
    }
    // option --output should be a file with yaml extension
    if (!outputFile.empty() && path::extension(outputFile) != ".yaml") {
        std::cerr << "Output file extension is not yaml" << '\n';
        std::cerr << "See clang-xform --help" << '\n';
        return 1;
    }
    // arguments for option --matchers should be registered
    MatcherFactory& factory = MatcherFactory::Instance();
    for (const auto& matcher : matchers) {
        if (!factory.CreateMatchers(matcher)) {
            // matcher id does not exist
            std::cerr << "Matcher ID: " << matcher << " is not registered!\n";
            return 1;
        }
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
        fs::real_path(compileCommands, tmp_path, true);
        fs::make_absolute(tmp_path);
        compileCommands = tmp_path.str().str();
    }
    if (!outputFile.empty()) {
        fs::real_path(outputFile, tmp_path, true);
        fs::make_absolute(tmp_path);
        outputFile = tmp_path.str().str();
    }
    if (!replaceFile.empty()) {
        fs::real_path(replaceFile, tmp_path, true);
        fs::make_absolute(tmp_path);
        replaceFile = tmp_path.str().str();
    }
    if (!logFile.empty()) {
        fs::real_path(logFile, tmp_path, true);
        fs::make_absolute(tmp_path);
        logFile = tmp_path.str().str();
    }
    for(auto& file : inputFiles) {
        fs::real_path(file, tmp_path, true);
        fs::make_absolute(tmp_path);
        file = tmp_path.str().str();
    }

    // print out version number
    if (version) {
        std::cout << argv[0] << " version "
                  << CLANG_XFORM_VERSION_MAJOR << '.'
                  << CLANG_XFORM_VERSION_MINOR << '.'
                  << CLANG_XFORM_VERSION_PATCH << '\n';
        return 0;
    }
    // display registered matchers
    if (display) {
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
        if (!replace::applyReplacements(replaceFile)) {
            std::cerr << "Failed in applying replacements in " + replaceFile << '\n';
            return 1;
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
        status = ProcessFiles(*compilations, inputFiles, outputFile, matchers, numThreads);
    }
    else
    {
        if (!inputFiles.empty()) {
            if (compilations) {
                // use fixedCompilationDatabase provided in command line
                status = ProcessFiles(*compilations, inputFiles, outputFile, matchers, numThreads);
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
                status = ProcessFiles(*compilations, inputFiles, outputFile, matchers, numThreads);
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
        if (!replace::applyReplacements(outputFile)) {
            std::cerr << "Failed in applying replacements in " + outputFile << '\n';
            return 1;
        }
    } else {
        std::cout << '\n' << "Replacements are stored in " << outputFile << "\n\n";
        std::cout << "To apply replacements, run:" << "\n\n";
        std::cout << "clang-xform -a " + outputFile << '\n';
    }

    std::cout << '\n' << "Check " << logFile << " to see log information" << "\n\n";
    
    return status;
}
