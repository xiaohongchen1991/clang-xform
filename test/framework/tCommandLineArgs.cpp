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

#include "CommandLineArgs.hpp"

#include <fstream>
#include <cstdio>

#include "gtest/gtest.h"

TEST(CommandLineArgsTest, AdjustCommandLineArgs) {
  CommandLineArgs args;
  args.matchers.push_back("m1");
  args.inputFiles.push_back("f1");
  args.configFile = "tmp.cfg";
  std::ofstream ofs(args.configFile);
  ofs << "matchers =m2\n"
      << "input-files= f2";
  ofs.close();
  AdjustCommandLineArgs(args);
  // remove tmp config file
  remove(args.configFile.c_str());
  std::vector<std::string> bmatchers = {"m1", "m2"};
  std::vector<std::string> bfiles = {"f1", "f2"};
  EXPECT_EQ(args.matchers, bmatchers);
  EXPECT_EQ(args.inputFiles, bfiles);
}

TEST(CommandLineArgsTest, ValidateCommandLineArgs) {
  // 1. test mutually exclusive flags
  std::string errmsg;
  // args1: sbcodexform --compile-commands compdb.json --
  constexpr int argc1 = 3;
  const char* argv1[argc1] = {"sbcodexform", "--compile-commands", "compdb.json"};
  auto args1 = ProcessCommandLine(argc1, const_cast<char**>(argv1));
  EXPECT_FALSE(ValidateCommandLineArgs(args1, true, errmsg));
  // args2: sbcodexform --apply output.yaml -d
  constexpr int argc2 = 4;
  const char* argv2[argc2] = {"sbcodexform", "--apply", "output.yaml", "-d"};
  auto args2 = ProcessCommandLine(argc2, const_cast<char**>(argv2));
  EXPECT_FALSE(ValidateCommandLineArgs(args2, false, errmsg));

  // 2. test yaml file extensions
  // args5: sbcodexform -a output.xml
  constexpr int argc3 = 3;
  const char* argv3[argc3] = {"sbcodexform", "-a", "output.xml"};
  auto args3 = ProcessCommandLine(argc3, const_cast<char**>(argv3));
  EXPECT_FALSE(ValidateCommandLineArgs(args3, false, errmsg));
  // args4: sbcodexform --output output -p compile_commands.json
  constexpr int argc4 = 5;
  const char* argv4[argc4] = {"sbcodexform", "--output", "output", "-p", "compile_commands.json"};
  auto args4 = ProcessCommandLine(argc4, const_cast<char**>(argv4));
  EXPECT_FALSE(ValidateCommandLineArgs(args4, false, errmsg));

  // 3. test unregisted matcher
  // args5: sbcodexform -p compile_commands.json --matchers m1
  constexpr int argc5 = 5;
  const char* argv5[argc5] = {"sbcodexform", "-p", "compile_commands.json", "--matchers", "m1"};
  auto args5 = ProcessCommandLine(argc5, const_cast<char**>(argv5));
  EXPECT_FALSE(ValidateCommandLineArgs(args5, false, errmsg));
}
