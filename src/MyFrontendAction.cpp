#include "MyFrontendAction.hpp"
#include "Logger.hpp"
#include "ToolingUtil.hpp"
#include "MatcherFactory.hpp"
#include "MyReplacementsYaml.hpp"

#include "clang/Tooling/Tooling.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/YAMLTraits.h"

#include <iostream>
#include <system_error>

#include <boost/assert.hpp>

using namespace clang;
using namespace llvm;
using namespace llvm::sys;
using namespace clang::ast_matchers;
using namespace clang::tooling;

std::mutex MyFrontendAction::mMutex;

MyFrontendAction::MyFrontendAction(const std::string& outputFile,
                                   const std::vector<std::string>& ids)
    : mOutputFile(outputFile)
{    
    // register AST Matchers with callback functions
    MatcherFactory& factory = MatcherFactory::Instance();
    for (const auto& id : ids) {
        mCallbacks.push_back(factory.CreateCallback(id, mReplacements));
        const auto matchers_ptr = factory.CreateMatchers(id);
        BOOST_ASSERT(matchers_ptr);
        const auto& matchers = *matchers_ptr;
        for (const auto& matcher : matchers) {
            mFinder.addDynamicMatcher(matcher,
                                      mCallbacks.back().get());            
        } 
    }
}

bool MyFrontendAction::BeginSourceFileAction (CompilerInstance &CI) {
    SourceManager& srcMgr = CI.getSourceManager();
    FileID fileID = srcMgr.getMainFileID();
    const FileEntry* fileEntry = srcMgr.getFileEntryForID(fileID);
    SmallString<256> tmp_path(fileEntry->getName());
    fs::make_absolute(tmp_path);
    const std::string fileName = tmp_path.str().str();
    TRIVIAL_LOG(info) << "Processing file: " << fileName << '\n';
    return true;
}

void MyFrontendAction::EndSourceFileAction() {
    // see https://github.com/llvm-mirror/clang/blob/master/tools/clang-rename/ClangRename.cpp
    // return if no replacements
    if (mReplacements.empty()) return;
    std::error_code EC;

    std::lock_guard<std::mutex> guard(mMutex);
    llvm::raw_fd_ostream OS(mOutputFile.get(), EC, llvm::sys::fs::F_Append);
    if (EC) {
        llvm::errs() << "Error opening output file: " << EC.message() << '\n';
        return;
    }

    tooling::TranslationUnitReplacements TUR;
    TUR.Replacements.insert(TUR.Replacements.end(),
                            mReplacements.begin(),
                            mReplacements.end());

    yaml::Output YAML(OS);
    YAML << TUR;
    OS.close();
    mReplacements.clear();
}
