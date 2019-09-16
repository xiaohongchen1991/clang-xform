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

#include "ToolingUtil.hpp"
#include "cxxlog.hpp"

#include <string>

#include "clang/Tooling/Tooling.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Lex/Lexer.h"

using namespace cxxlog;
using namespace clang;

clang::SourceLocation getExpansionLocStart(clang::SourceLocation Loc, const clang::SourceManager& sm)
{
  if (Loc.isFileID()) return Loc;
  else {
    do {
      Loc = sm.getImmediateExpansionRange(Loc).getBegin();
      // Loc = sm.getImmediateExpansionRange(Loc).first;
    } while (!Loc.isFileID());
    return Loc;
  }
}

SourceLocation getExpansionLocEnd(SourceLocation Loc, const SourceManager& sm)
{
  if (Loc.isFileID()) return Loc;
  else {
    do {
      if (sm.isMacroArgExpansion(Loc))
        Loc = sm.getImmediateExpansionRange(Loc).getBegin();
      // Loc = sm.getImmediateExpansionRange(Loc).first;
      else
        Loc = sm.getImmediateExpansionRange(Loc).getEnd();
      // Loc = sm.getImmediateExpansionRange(Loc).second;
    } while (!Loc.isFileID());
    return Loc;
  }
}

std::string getSourceText(SourceLocation Start,
                          SourceLocation End,
                          const SourceManager& SM,
                          const LangOptions &LangOpts)
{
  End = Lexer::getLocForEndOfToken(End, 0, SM, LangOpts);
  //CharSourceRange Range = Lexer::getAsCharRange(SourceRange(std::move(Start), std::move(End)), SM, LangOpts);
  //return Lexer::getSourceText(Range ,SM, LangOpts);
  return std::string(SM.getCharacterData(Start),
                     SM.getCharacterData(End) - SM.getCharacterData(Start));
}

void LogReplacement(clang::SourceLocation loc, const clang::SourceManager& sm,
                    const std::string& oldExpr, const std::string& newExpr)
{
  std::string screenLogString =  "Editting file: " + loc.printToString(sm) + ": \"" + oldExpr + "\"" +
      " --> " + "\"" + newExpr + "\"";
  std::string fileLogString =  "Editting file:\n" + loc.printToString(sm) + ":\n\"" + oldExpr + "\"" +
      " --> " + "\"" + newExpr + "\"";
  TRIVIAL_LOG(severity::info) << screenLogString << '\n';
  FILE_LOG(severity::info) << fileLogString << "\n\n";
}

void LogASTNode(clang::SourceLocation loc, const clang::SourceManager& sm,
                const std::string& expr) {
  std::string screenLogString =  "Finding AST Node: " +
      loc.printToString(sm) + ": \"" + expr + "\"";
  std::string fileLogString =  "Finding AST Node:\n" +
      loc.printToString(sm) + ":\n\"" + expr + "\"";
    
  TRIVIAL_LOG(info) << screenLogString << '\n';
  FILE_LOG(info) << fileLogString << "\n\n";
}
