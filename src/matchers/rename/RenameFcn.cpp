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

#include "MatchCallbackDef.hpp"  // provides macro MATCH_CALLBACK to define a new match callback
#include "MatcherFactory.hpp"   // a factory class used to register new matcher and callback
#include "MatcherHelper.hpp"
#include "ToolingUtil.hpp"      // APIs to extract locations and tokens for a given AST node

#include <string>

using namespace clang;
using namespace clang::ast_matchers;

namespace {
// option1: qualified function name to match
// option2: new function name to use
const std::string option1 = "qualified-name";
const std::string option2 = "new-name";

// Match callback class RenameFcnCallback is defined here
OPTION_MATCH_CALLBACK(RenameFcnCallback);

void RenameFcnCallback::RegisterOptions() {
  AddOption<std::string>(option1);
  AddOption<std::string>(option2);
}

void RenameFcnCallback::RegisterMatchers(clang::ast_matchers::MatchFinder* finder) {
  StatementMatcher RenameFcnMatcher =
      callExpr(callee(functionDecl(hasName(GetOption<std::string>(option1)))),
               isExpansionInMainFile()
               ).bind("RenameFcnExpr");

  finder->addMatcher(RenameFcnMatcher, this);
}

// Definiation of RenameFcnCallback::run
void RenameFcnCallback::run(const clang::ast_matchers::MatchFinder::MatchResult& Result) {
  std::string oldExprString;
  std::string newExprString;
  const auto& srcMgr = Result.Context->getSourceManager();
  const auto& langOpts = Result.Context->getLangOpts();

  // Check any AST node matched for the given string ID.
  // The node class name is usually the capitalized node matcher name.
  if (const CallExpr* RenameFcnExpr =
      Result.Nodes.getNodeAs<CallExpr>("RenameFcnExpr")) {
    // find begin and end file locations of a given node
    // use getExprLoc() for the begin loc which returns MemberLoc if it is a member function.
    // i.e. X->F return F
    auto locStart = srcMgr.getFileLoc(RenameFcnExpr->getCallee()->getExprLoc());
    auto locEnd = srcMgr.getFileLoc(RenameFcnExpr->getCallee()->getEndLoc());
    newExprString = GetOption<std::string>(option2);
    // find source text for a given location
    oldExprString = getSourceText(locStart, locEnd, srcMgr, langOpts);
    // replace source text with a given string
    ReplaceText(srcMgr, SourceRange(std::move(locStart), std::move(locEnd)), newExprString);
    // log the replacement or AST node if no replacement is made
    LogReplacement(locStart, srcMgr, oldExprString, newExprString);
  }
}

// register your own matcher and callback
MatcherHelper<RenameFcnCallback> RegisterRenameFcnMatcher("RenameFcn");

}
