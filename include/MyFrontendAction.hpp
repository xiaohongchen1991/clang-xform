#ifndef MY_FRONTEND_ACTION_HPP
#define MY_FRONTEND_ACTION_HPP

#include <vector>
#include <memory>
#include <mutex>
#include <string>

#include "clang/Frontend/FrontendActions.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Core/Replacement.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"

class MyFrontendAction : public clang::ASTFrontendAction
{
  public:
    explicit MyFrontendAction(const std::string& outputFile,
                              const std::vector<std::string>& matchers);

  protected:
    virtual std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) override
    {
         return mFinder.newASTConsumer();
    }
    virtual bool BeginSourceFileAction (clang::CompilerInstance &CI) override;
    virtual void EndSourceFileAction() override;
  private:
    std::reference_wrapper<const std::string> mOutputFile;
    clang::ast_matchers::MatchFinder mFinder;
    clang::tooling::Replacements mReplacements;
    std::vector<std::unique_ptr<clang::ast_matchers::MatchFinder::MatchCallback> > mCallbacks;
    static std::mutex mMutex;
};

class MyFrontendActionFactory : public clang::tooling::FrontendActionFactory {
  public:
    MyFrontendActionFactory(const std::string& outputFile, const std::vector<std::string>& matchers)
        : mOutputFile(outputFile), mMatchers(matchers) {}
    
    clang::FrontendAction *create() override { return new MyFrontendAction(mOutputFile.get(), mMatchers.get()); }
  private:
    std::reference_wrapper<const std::string> mOutputFile;
    std::reference_wrapper<const std::vector<std::string> > mMatchers;
};


#endif
