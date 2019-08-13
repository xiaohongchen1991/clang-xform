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
