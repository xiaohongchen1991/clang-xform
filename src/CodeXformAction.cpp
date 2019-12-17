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

#include "CodeXformAction.hpp"
#include "cxxlog.hpp"
#include "ToolingUtil.hpp"
#include "MatcherFactory.hpp"
#include "MyReplacementsYaml.hpp"
#include "CommandLineArgsUtil.hpp"

#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/YAMLTraits.h"

#include <iostream>
#include <system_error>

using namespace cxxlog;
using namespace clang;
using namespace llvm;
using namespace llvm::sys;
using namespace clang::ast_matchers;
using namespace clang::tooling;

std::mutex CodeXformAction::mMutex;

CodeXformAction::CodeXformAction(const std::string& outputFile,
                                 const std::vector<std::string>& ids,
                                 const std::vector<std::string>& args)
    : mOutputFile(outputFile)
{
  // register command line options for each MatchCallback
  MatcherFactory& factory = MatcherFactory::Instance();
  for (const auto& id : ids) {
    auto matcher_args = GetMatcherArgs(args, id);
    if (matcher_args.empty()) {
      // matcher has no arguments
      mCallbacks.push_back(factory.CreateMatchCallback(id, mReplacements,
                                                       std::vector<std::string>()));
      assert(mCallbacks.back());
      // register command line options and matchers
      mCallbacks.back()->Register(&mFinder);
    } else {
      // create multiple instances of the given matcher with different arguments setting
      for (auto& args : matcher_args) {
        mCallbacks.push_back(factory.CreateMatchCallback(id, mReplacements,
                                                         std::move(args)));
        assert(mCallbacks.back());
        // register command line options and matchers
        mCallbacks.back()->Register(&mFinder);
      }
    }
  }
}

bool CodeXformAction::BeginSourceFileAction (CompilerInstance &CI) {
  TRIVIAL_LOG(info) << "Processing file: " << getCurrentFile().str() << '\n';
  return true;
}

void CodeXformAction::EndSourceFileAction() {
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
  TUR.MainSourceFile = getCurrentFile().str();
  TUR.Replacements.insert(TUR.Replacements.end(),
                          mReplacements.begin(),
                          mReplacements.end());

  yaml::Output YAML(OS);
  YAML << TUR;
  OS.close();
  mReplacements.clear();
}
