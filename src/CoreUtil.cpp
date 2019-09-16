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

#include "CoreUtil.hpp"
#include "CodeXformException.hpp"
#include "DiagnosticLogger.hpp"
#include "CodeXformActionFactory.hpp"

#include <array>
#include <sstream>
#include <fstream>
#include <cassert>
#include <thread>
#include <future>
#include <numeric>

#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "llvm/Support/raw_os_ostream.h"

using namespace clang::tooling;
using namespace clang;
using namespace llvm;

namespace {

// remove whitespace for the given string
inline std::string RemoveWhitespace(const std::string& s)
{
  auto begin = s.find_first_not_of(' ');
  auto end = s.find_last_not_of(' ');
  assert((begin != std::string::npos) && (end != std::string::npos));
  return s.substr(begin, end + 1 - begin);
}

} // end anonymous namespace

int ExecCmd(const std::string& cmd, std::string& result) {
  std::array<char, 128> buffer;
  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe) {
    throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
    result += buffer.data();
  }
  return WEXITSTATUS(pclose(pipe));
}

std::vector<std::string> ParseConfigFile(const std::string& fileName, const std::string& key)
{
  // open file for reading
  std::ifstream ifs(fileName);
  std::string line;
  std::vector<std::string> args;
    
  if (ifs.good()) {
    while (std::getline(ifs, line)) {
      std::istringstream is_line(line);
      std::string next_key;
      if (std::getline(is_line, next_key, '=') || std::getline(is_line, next_key, ' ')) {
        // check if the next_key string is the same as input key string after removing extra white spaces
        next_key = RemoveWhitespace(next_key);
        if (next_key != key) continue;
        std::string value;
        while (std::getline(is_line, value, ' ')) {
          if (value.empty()) {
            continue;
          }
          value = RemoveWhitespace(value);
          args.emplace_back(std::move(value));
        }
      }
    }
  } else {
    throw FileSystemException("Cannot open file: " + fileName);
  }
  return args;
}

int ProcessFiles(const CompilationDatabase& compilationDatabase,
                 const std::vector<std::string>& inputFiles,
                 const std::string& outputFile,
                 const std::vector<std::string>& matchers,
                 const std::vector<const char*>& matcherArgs,
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
    
  std::vector<std::thread> threads;
  std::vector<std::future<std::tuple<int, std::string> > > futures;

  // loop through the list of files and process filesPerCore during each loop iteration.
  for (size_t beginRange = 0, endRange = std::min(numFiles, filesPerCore); beginRange < numFiles;
       beginRange = endRange, endRange = std::min(numFiles, endRange + filesPerCore)) {

    std::vector<std::string> fileSubset(inputFiles.begin() + beginRange,
                                        inputFiles.begin() + endRange);

    std::packaged_task<std::tuple<int, std::string>()> task(
        [&compilationDatabase, &outputFile, &matchers, &matcherArgs, files = std::move(fileSubset)]()
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

          const int toolStatus = tool.run(std::make_unique<CodeXformActionFactory>(outputFile, matchers, matcherArgs).get());

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
                                 throw RunClangToolException(nextErrorMessage);
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
