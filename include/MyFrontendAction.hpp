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
