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

#ifndef CODE_XFORM_ACTION_FACTORY_HPP
#define CODE_XFORM_ACTION_FACTORY_HPP

#include <string>
#include <vector>

#include "clang/Tooling/Tooling.h"

// forward declaration
namespace clang {
class FrontendAction;
} // end of namespace clang

class CodeXformActionFactory : public clang::tooling::FrontendActionFactory {
 public:
  CodeXformActionFactory(const std::string& outputFile,
                         const std::vector<std::string>& matchers,
                         const std::vector<std::string>& matcherArgs)
      : mOutputFile(outputFile),
        mMatchers(matchers),
        mMatcherArgs(matcherArgs)
  {}

  clang::FrontendAction *create() override;

 private:
  std::reference_wrapper<const std::string> mOutputFile;
  std::reference_wrapper<const std::vector<std::string> > mMatchers;
  std::reference_wrapper<const std::vector<std::string> > mMatcherArgs;
};


#endif
