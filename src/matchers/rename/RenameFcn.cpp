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

#include "MyMatchCallback.hpp"  // provides macro MATCH_CALLBACK to define a new match callback
#include "MatcherFactory.hpp"   // a factory class used to register new matcher and callback
#include "ToolingUtil.hpp"      // APIs to extract locations and tokens for a given AST node 
#include "Logger.hpp"           // basic logging functionality

#include <string>

using namespace clang;
using namespace clang::ast_matchers;

// This a generated template file to help write your own clang AST matcher and callback.
// Please address all comments in /**/ form below!
// [1] : use StatementMatcher or DeclarationMatcher
// [2] : replace it with a suitable node matcher
// [3] : add narrowing or traversal matchers here
// [4] : rename the string ID for a better description of the matched node
// [5] : replace it with the corresponding node class type binded with the string ID
// [6] : variable name for the matched node
// [7] : same as [5]
// [8] : string ID used in matcher, same as [4]
// [9] : name of the node to be replaced
// [10]: string used to replace the matched node
// [11]: use "LogASTNode(locStart, srcMgr, oldExprString)" to log matched AST Node infomation
// [12]: can register more than one matcher of different types

namespace {
// oldName: qualified function name to match
// newName: new function name to use
const std::string oldName = "Foo";
const std::string newName = "Bar";
    
StatementMatcher RenameFcnMatcher =
    callExpr(callee(functionDecl(hasName(oldName))),
             isExpansionInMainFile()
        ).bind("RenameFcnExpr");

// Match callback class RenameFcnCallback is defined here
MATCH_CALLBACK(RenameFcnCallback);

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
        newExprString = newName;
        // find source text for a given location
        oldExprString = getSourceText(locStart, locEnd, srcMgr, langOpts);
        // replace source text with a given string
        ReplaceText(srcMgr, SourceRange(std::move(locStart), std::move(locEnd)), newExprString);
        // log the replacement or AST node if no replacement is made
        LogReplacement(locStart, srcMgr, oldExprString, newExprString);
    }
}

// register your own matcher and callback
MatcherHelper<RenameFcnCallback> RegisterRenameFcnMatcher("RenameFcn", {RenameFcnMatcher});

}
