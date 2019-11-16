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

#include "MatchCallbackBase.hpp"

#include "clang/Tooling/Inclusions/HeaderIncludes.h"
#include "clang/Tooling/Inclusions/IncludeStyle.h"

using namespace llvm;
using namespace clang;
using namespace clang::tooling;

// if there is an existing header with match the regex, insert there,
// otherwise, following this rule:
// #include "...."
// #include "header"
// #include <...>
llvm::Optional<clang::SourceLocation> MatchCallbackBase::InsertHeader(const clang::SourceManager& srcMgr,
                                                                      const clang::FileID& fileID,
                                                                      llvm::StringRef header,
                                                                      llvm::StringRef regex) {
  const FileEntry* fileEntry = srcMgr.getFileEntryForID(fileID);
  auto fileBuffer = srcMgr.getBufferData(fileID);
  IncludeStyle style;
  style.IncludeBlocks = IncludeStyle::IBS_Regroup;
  IncludeStyle::IncludeCategory cat_custom;
  cat_custom.Regex = regex.str();
  cat_custom.Priority = 2;
  IncludeStyle::IncludeCategory cat_default;
  cat_default.Regex = "^\"";
  cat_default.Priority = 1;
  IncludeStyle::IncludeCategory cat_system;
  cat_system.Regex = "^<";
  cat_system.Priority = 3;
  style.IncludeCategories.push_back(cat_custom);
  style.IncludeCategories.push_back(cat_default);
  style.IncludeCategories.push_back(cat_system);
  llvm::Optional<clang::SourceLocation> loc;
  if (auto replacement = HeaderIncludes(fileEntry->getName(), fileBuffer, style)
      .insert(header, false)) {
    MergeReplacement(replacement.getValue());
    loc = srcMgr.getComposedLoc(fileID, replacement->getOffset());
  }
  return loc;
}
