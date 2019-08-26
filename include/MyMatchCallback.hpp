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

#ifndef MY_MATCH_CALLBACK_HPP
#define MY_MATCH_CALLBACK_HPP

#include "cxxopts.hpp"

#include <functional>
#include <iostream>

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Core/Replacement.h"

class MyMatchCallback : public clang::ast_matchers::MatchFinder::MatchCallback {
 public :
  explicit MyMatchCallback(const std::string& id,
                           clang::tooling::Replacements& replacements,
                           std::vector<const char*> args)
      : mOptions(id), mReplacements(replacements), mArgs(std::move(args))
  {}

  virtual ~MyMatchCallback() {}

  virtual void RegisterMatchers(clang::ast_matchers::MatchFinder* finder) = 0;

  llvm::Error AddReplacement(const clang::tooling::Replacement& R) {
    return mReplacements.get().add(R);
  }
  // return 0 for success
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

  virtual void RegisterOptions() {
    // default do nothing
  }

 protected:
  template <typename T>
  void AddOption(const std::string& key) {
    mOptions.add_options()(key, "", cxxopts::value<T>());
  }

  template <typename T>
  void AddOption(const std::string& key, T default_value) {
    mOptions.add_options()(key, "", cxxopts::value<T>(default_value));
  }

  template <typename T>
  T GetOption(const std::string& key) {
    static int argc = static_cast<int>(mArgs.size());
    static char** argv = const_cast<char**>(&mArgs[0]);
    static cxxopts::ParseResult result = mOptions.parse(argc, argv);
    if (!result.count(key)) {
      std::cerr << "Option " + key + " is registered, but no corresponding command line argument is provided!" << '\n';
      exit(1);
    }
    assert(result.count(key));
    return result[key].as<T>();
  }

 private:
  cxxopts::Options mOptions;
  std::reference_wrapper<clang::tooling::Replacements> mReplacements;
  std::vector<const char*> mArgs;
};


#define MATCH_CALLBACK(NAME)                                                               \
  class NAME : public MyMatchCallback {                                                    \
   public :                                                                                \
   explicit NAME (const std::string& id,                                                   \
                  clang::tooling::Replacements& replacements,                              \
                  std::vector<const char*> args)                                           \
       : MyMatchCallback(id, replacements, std::move(args))                                \
    {}                                                                                     \
   virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) override; \
   virtual void RegisterMatchers(clang::ast_matchers::MatchFinder* finder) override;       \
  }

#define OPTION_MATCH_CALLBACK(NAME)                                                        \
  class NAME : public MyMatchCallback {                                                    \
   public :                                                                                \
   explicit NAME (const std::string& id,                                                   \
                  clang::tooling::Replacements& replacements,                              \
                  std::vector<const char*> args)                                           \
       : MyMatchCallback(id, replacements, std::move(args))                                \
    {}                                                                                     \
   virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) override; \
   virtual void RegisterMatchers(clang::ast_matchers::MatchFinder* finder) override;       \
   virtual void RegisterOptions() override;                                                \
  }

#endif
