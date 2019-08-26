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

#include "MyFrontendAction.hpp"
#include "Logger.hpp"
#include "ToolingUtil.hpp"
#include "MatcherFactory.hpp"
#include "MyReplacementsYaml.hpp"

#include "clang/Tooling/Tooling.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/YAMLTraits.h"

#include <iostream>
#include <system_error>

#include <boost/assert.hpp>

using namespace clang;
using namespace llvm;
using namespace llvm::sys;
using namespace clang::ast_matchers;
using namespace clang::tooling;

std::mutex MyFrontendAction::mMutex;

namespace {

std::vector<const char*> GetMatcherArgs(const std::vector<const char*>& args,
                                        const std::string& id) {
  std::vector<const char*> strippedArgs;
  if (args.empty()) {
    return strippedArgs;
  }

  auto begin = std::find_if(args.begin(), args.end(),
                            [&id](auto str){return (strstr(str, id.c_str()) != nullptr);});

  if (begin == args.end()) {
    return strippedArgs;
  }

  auto end = std::find_if(begin + 1, args.end(),
                          [](auto str){return (strstr(str, "matcher-args") != nullptr);});

  // the first arg will de discarded in cxxopt
  strippedArgs.reserve(end - begin);
  strippedArgs.insert(strippedArgs.end(), begin, end);

  return strippedArgs;
}

} // end of anonymous namespace

MyFrontendAction::MyFrontendAction(const std::string& outputFile,
                                   const std::vector<std::string>& ids,
                                   const std::vector<const char*>& args)
    : mOutputFile(outputFile)
{
  // register command line options for each MatchCallback
  MatcherFactory& factory = MatcherFactory::Instance();
  for (const auto& id : ids) {
    mCallbacks.push_back(factory.CreateMatchCallback(id, mReplacements,
                                                     GetMatcherArgs(args, id)));
    assert(mCallbacks.back());
    // register command line options
    mCallbacks.back()->RegisterOptions();
    // register AST Matchers with callback functions
    mCallbacks.back()->RegisterMatchers(&mFinder);
  }
}

bool MyFrontendAction::BeginSourceFileAction (CompilerInstance &CI) {
  SourceManager& srcMgr = CI.getSourceManager();
  FileID fileID = srcMgr.getMainFileID();
  const FileEntry* fileEntry = srcMgr.getFileEntryForID(fileID);
  SmallString<256> tmp_path(fileEntry->getName());
  fs::make_absolute(tmp_path);
  const std::string fileName = tmp_path.str().str();
  TRIVIAL_LOG(info) << "Processing file: " << fileName << '\n';
  return true;
}

void MyFrontendAction::EndSourceFileAction() {
  // see https://github.com/llvm-mirror/clang/blob/master/tools/clang-rename/ClangRename.cpp
  // return if no replacements
  if (mReplacements.empty()) return;
  std::error_code EC;

  std::lock_guard<std::mutex> guard(mMutex);
  llvm::raw_fd_ostream OS(mOutputFile.get(), EC, llvm::sys::fs::F_Append);
  if (EC) {
    llvm::errs() << "Error opening output file: " << EC.message() << '\n';
    return;
  }

  tooling::TranslationUnitReplacements TUR;
  TUR.Replacements.insert(TUR.Replacements.end(),
                          mReplacements.begin(),
                          mReplacements.end());

  yaml::Output YAML(OS);
  YAML << TUR;
  OS.close();
  mReplacements.clear();
}
