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

#ifndef MATCH_CALLBACK_BASE_HPP
#define MATCH_CALLBACK_BASE_HPP

#include "cxxopts.hpp"
#include "CodeXformException.hpp"

#include <iostream>
#include <memory>

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Core/Replacement.h"

class MatchCallbackBase : public clang::ast_matchers::MatchFinder::MatchCallback {
 public :
  explicit MatchCallbackBase(const std::string& id,
                             clang::tooling::Replacements& replacements,
                             std::vector<std::string> args)
      : mOptions(id), mReplacements(replacements), mArgs(std::move(args))
  {
    if (!mArgs.empty() && (("--matcher-args-" + id) != mArgs[0])) {
      throw CommandLineOptionException("Cannot find matcher arguments separator --matcher-args-" + id);
    }
  }

  virtual ~MatchCallbackBase() {}

  llvm::Error AddReplacement(const clang::tooling::Replacement& R) {
    return mReplacements.get().add(R);
  }

  void MergeReplacement(const clang::tooling::Replacement& R) {
    mReplacements.get() = mReplacements.get().merge(clang::tooling::Replacements(R));
  }

  /*
   *  Abstract: Insert text NewStr in the Start location. Allow deplication and conflicts
   */
  void InsertText(const clang::SourceManager &Sources, clang::SourceLocation Start,
                  llvm::StringRef NewStr) {
    return MergeReplacement(clang::tooling::Replacement(Sources,
                                                        Start,
                                                        0,
                                                        NewStr));
  }

  /*
   * Abstract: Insert a new header in the file with the given fileID and
   *           group the headers with same regex.
   *           If the header already exists, ignore it.
   *           Return a SourceLocation for logging purpose.
   */
  llvm::Optional<clang::SourceLocation> InsertHeader(const clang::SourceManager& srcMgr,
                                                     const clang::FileID& fileID,
                                                     llvm::StringRef header,
                                                     llvm::StringRef regex);

  /*
   *  Abstract: Replace text marked by the given range by the NewStr.
   *            Conflicts and most duplicates will be discarded.
   *            One exception: duplicates with "OrigLength == 0" will be merged
   */
  llvm::Error ReplaceText (const clang::SourceManager &Sources, clang::SourceLocation Start,
                           unsigned OrigLength, llvm::StringRef NewStr) {
    return AddReplacement(clang::tooling::Replacement(Sources,
                                                      Start,
                                                      OrigLength,
                                                      NewStr));
  }
  llvm::Error ReplaceText (const clang::SourceManager &Sources,
                           clang::SourceRange range,
                           llvm::StringRef NewStr,
                           const clang::LangOptions &LangOpts=clang::LangOptions()) {
    return AddReplacement(clang::tooling::Replacement(Sources,
                                                      clang::CharSourceRange::getTokenRange(range),
                                                      NewStr,
                                                      LangOpts));
  }

  virtual void RegisterMatchers(clang::ast_matchers::MatchFinder* finder) = 0;

  virtual void RegisterOptions() {
    // default do nothing
  }

  virtual void ParseOptions() {
    if (!mArgs.empty()) {
      int argc = static_cast<int>(mArgs.size());
      std::vector<char*> cstrArgs;
      cstrArgs.reserve(argc);
      for (auto& s : mArgs) {
        cstrArgs.push_back(const_cast<char*>(s.c_str()));
      }
      char** argv = &cstrArgs[0];
      mResult = std::make_unique<cxxopts::ParseResult>(mOptions.parse(argc, argv));
    }
  }

  // register options and matchers
  void Register(clang::ast_matchers::MatchFinder* finder) {
    // 1. register options
    RegisterOptions();
    // 2. parse options
    ParseOptions();
    // 3. register matchers
    RegisterMatchers(finder);
  }

 protected:
  template <typename T>
  void AddOption(const std::string& key) {
    mOptions.add_options()(key, "", cxxopts::value<T>());
  }

  template <typename T>
  void AddOption(const std::string& key, const std::string& value) {
    mOptions.add_options()(key, "", cxxopts::value<T>()->default_value(value));
  }

  template <typename T>
  T GetOption(const std::string& key) const {
    assert(mResult != nullptr);
    return (*mResult.get())[key].as<T>();
  }

 private:
  cxxopts::Options mOptions;
  // initialization of cxxopts::ParseResult needs to be delayed.
  // Use heap memory for now. May switch to std::optional if c++17 is supported
  std::unique_ptr<cxxopts::ParseResult> mResult;
  std::reference_wrapper<clang::tooling::Replacements> mReplacements;
  std::vector<std::string> mArgs;
};

#endif
