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

#include <functional>

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Core/Replacement.h"

class MyMatchCallback : public clang::ast_matchers::MatchFinder::MatchCallback {
  public :
    explicit MyMatchCallback(clang::tooling::Replacements& replacements)
        : mReplacements(replacements)
    {}

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
    
  private:
    std::reference_wrapper<clang::tooling::Replacements> mReplacements;
};

    
#define MATCH_CALLBACK(NAME)                                                                \
class NAME : public MyMatchCallback {                                                       \
  public :                                                                                  \
    explicit NAME (clang::tooling::Replacements& replacements)                              \
        : MyMatchCallback(replacements)                                                     \
    {}                                                                                      \
    virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) override; \
}

#endif
