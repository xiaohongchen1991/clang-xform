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

#include "TestingUtil.hpp"
#include "Logger.hpp"
#include "ToolingUtil.hpp"

#include <fstream>
#include <string>
#include <limits.h>
#include <unistd.h>

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"

using namespace llvm;
using namespace llvm::sys;

bool CompareFiles(const std::string& p1, const std::string& p2) {
    std::ifstream f1(p1, std::ifstream::binary|std::ifstream::ate);
    std::ifstream f2(p2, std::ifstream::binary|std::ifstream::ate);
    
    if (f1.fail() || f2.fail()) {
        return false; //file problem
    }
    
    if (f1.tellg() != f2.tellg()) {
        return false; //size mismatch
    }

    //seek back to beginning and use std::equal to compare contents
    f1.seekg(0, std::ifstream::beg);
    f2.seekg(0, std::ifstream::beg);
    return std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
                      std::istreambuf_iterator<char>(),
                      std::istreambuf_iterator<char>(f2.rdbuf()));
}

bool InitTest(const std::string& dirPath,
              const std::string& outputFile) {
    SmallString<256> tmp_path;
    // get exe path
    char result[PATH_MAX];
    int count = readlink("/proc/self/exe", result, PATH_MAX);
    std::string root(result, (count > 0) ? count : 0);
    auto pos = root.find_last_of("/");
    if (pos == std::string::npos) return false;

    root = root.substr(0, pos);
    root = root + "/../..";
    fs::real_path(root, tmp_path);
    root = tmp_path.str();
    std::string testdir = root + '/' + dirPath;
  
    // cd to test directory
    fs::set_current_path(testdir);
    // create a new file for output
    if (fs::exists(outputFile)) {
        fs::remove(outputFile);
    }

    tmp_path = testdir;
    fs::createUniqueFile(outputFile, tmp_path);
    // setup logging
    FileLog::Verbosity() = verbosity::minimal;
    TrivialLog::Verbosity() = verbosity::quiet;
    // update compile_commands.json
    if (ExecCmd(root + "/scripts/update-compile-commands.py"))
        return false;

    return true;
}

bool IsEmptyFile(const std::string& file) {
    std::ifstream ifs("file");
    return ifs.peek() == std::ifstream::traits_type::eof();
}
