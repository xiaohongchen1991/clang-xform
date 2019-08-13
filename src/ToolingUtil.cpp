#include "ToolingUtil.hpp"
#include "Logger.hpp"

#include <array>
#include <vector>
#include <memory>
#include <stdexcept>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>

#include <boost/assert.hpp>

#include "clang/Tooling/Tooling.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Lex/Lexer.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

using namespace clang;
using namespace llvm;
using namespace llvm::sys;

namespace {

// remove whitespace for the given string
inline std::string RemoveWhitespace(const std::string& s)
{
    auto begin = s.find_first_not_of(' ');
    auto end = s.find_last_not_of(' ');
    assert((begin != std::string::npos) && (end != std::string::npos));
    return s.substr(begin, end + 1 - begin);
}

} // end anonymous namespace

int ExecCmd(const std::string& cmd, std::string& result) {
    std::array<char, 128> buffer;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    return WEXITSTATUS(pclose(pipe));
}

bool ParseConfigFile(const std::string& fileName, const std::string& key, std::vector<std::string>& args)
{
    BOOST_ASSERT(args.empty());
    // open file for reading
    std::ifstream ifs(fileName);
    std::string line;
    
    if (ifs.good()) {
        while (std::getline(ifs, line)) {
            std::istringstream is_line(line);
            std::string next_key;
            if (std::getline(is_line, next_key, '=') || std::getline(is_line, next_key, ' ')) {
                // check if the next_key string is the same as input key string after removing extra white spaces
                next_key = RemoveWhitespace(next_key);
                if (next_key != key) continue;
                std::string value;
                while (std::getline(is_line, value, ' ')) {
                    if (value.empty()) {
                        continue;
                    }
                    value = RemoveWhitespace(value);
                    args.emplace_back(std::move(value));
                }
            }
        }
    } else {
        std::cerr << "Cannot open file: " << fileName << '\n';
        return false;
    }
    return true;
}

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
