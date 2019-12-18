#include "MatchCallbackDef.hpp" // provides macro MATCH_CALLBACK to define a new match callback
#include "MatcherHelper.hpp"    // a factory class used to register new matcher and callback
#include "ToolingUtil.hpp"      // APIs to extract locations and tokens for a given AST node

#include <string>

using namespace clang;
using namespace clang::ast_matchers;

// This is a generated template file to help write your own clang AST matcher and callback.
// If you want to support command line options, use -o, --option flag for gen-option-matcher.py.
// Please address all comments in /**/ form below!
// [1] : use StatementMatcher or DeclarationMatcher
// [2] : replace it with a suitable node matcher
// [3] : add narrowing or traversal matchers here
// [4] : rename the string ID for a better description of the matched node
// [5] : replace it with the corresponding node class type bound with the string ID
// [6] : variable name for the matched node
// [7] : same as [5]
// [8] : string ID used in matcher, same as [4]
// [9] : name of the node to be replaced
// [10]: string used to replace the matched node
// [11]: Use ReplaceText() for replacement and InsertText() for insertion (ex. header insertion)
// [12]: use "LogASTNode(locBegin, srcMgr, oldExprString)" to log matched AST Node information

namespace {

// Match callback class __NAME__Callback is defined here
MATCH_CALLBACK(__NAME__Callback);

void __NAME__Callback::RegisterMatchers(clang::ast_matchers::MatchFinder* finder) {
  // Define your own matcher here.
  // Use StatementMatcher to match statements and DeclarationMatcher to match declarations.
  // It is recommended that isExpansionInMainFile() is used to avoid matches in
  // system headers or third-party libraries.
  // For AST matcher reference, see: https://clang.llvm.org/docs/LibASTMatchersReference.html
  // For AST matcher examples, check files under "matchers/" directory
  StatementMatcher/*[1]*/ __NAME__Matcher =
      expr/*[2]*/(/*[3]*/,
                  isExpansionInMainFile()
                  ).bind("__NAME__Expr"/*[4]*/);

  finder->addMatcher(__NAME__Matcher, this);
}

// Definition of __NAME__Callback::run
void __NAME__Callback::run(const clang::ast_matchers::MatchFinder::MatchResult& Result) {
  std::string oldExprString;
  std::string newExprString;
  const auto& srcMgr = Result.Context->getSourceManager();
  const auto& langOpts = Result.Context->getLangOpts();

  // Check any AST node matched for the given string ID.
  // The node class name is usually the capitalized node matcher name.
  if (const Expr*/*[5]*/ __NAME__Expr/*[6]*/ =
      Result.Nodes.getNodeAs<Expr/*[7]*/>("__NAME__Expr"/*[8]*/)) {
    // find begin and end file locations of a given node
    auto locBegin = srcMgr.getFileLoc(__NAME__Expr/*[9]*/->getBeginLoc());
    auto locEnd = srcMgr.getFileLoc(__NAME__Expr/*[9]*/->getEndLoc());
    newExprString = ""/*[10]*/;
    // find source text for a given location
    oldExprString = getSourceText(locBegin, locEnd, srcMgr, langOpts);
    // replace source text with a given string or use InsertText() to insert new text
    ReplaceText(srcMgr, SourceRange(std::move(locBegin), std::move(locEnd)), newExprString)/*[11]*/;
    // log the replacement or AST node if no replacement is made
    LogReplacement(locBegin, srcMgr, oldExprString, newExprString)/*[12]*/;
  }
}

// register your own matcher and callback
MatcherHelper<__NAME__Callback> Register__NAME__Matcher("__NAME__");

}
