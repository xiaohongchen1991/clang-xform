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

TEST(CommandLineArgsTest, AdjustCommandLineArgs_NonMatcherArgs) {
  CommandLineArgs args;
  args.matchers.push_back("m1");
  args.inputFiles.push_back("f1");
  args.configFile = "tmp.cfg";
  std::vector<std::string> matcherArgs;
  std::ofstream ofs(args.configFile);
  ofs << "matchers =m2\n"
      << "input-files= f2";
  ofs.close();
  AdjustCommandLineArgs(args, matcherArgs);
  // remove tmp config file
  remove(args.configFile.c_str());
  std::vector<std::string> bmatchers = {"m1", "m2"};
  std::vector<std::string> bfiles = {"f1", "f2"};

  EXPECT_EQ(args.matchers, bmatchers);
  EXPECT_EQ(args.inputFiles, bfiles);
  EXPECT_TRUE(matcherArgs.empty());
}

TEST(CommandLineArgsTest, AdjustCommandLineArgs_MatcherArgs) {
  CommandLineArgs args;
  args.configFile = "tmp.cfg";
  std::vector<std::string> matcherArgs = {"--matcher-args-m1", "--bc", "3"};
  std::ofstream ofs(args.configFile);
  ofs << "matcher-args-m2 --abc 2\n"
      << "matcher-args-m2 --ab 1";
  ofs.close();
  AdjustCommandLineArgs(args, matcherArgs);
  // remove tmp config file
  remove(args.configFile.c_str());
  std::vector<std::string> bmatcherArgs = {"--matcher-args-m1", "--bc", "3",
                                           "--matcher-args-m2", "--abc", "2",
                                           "--matcher-args-m2", "--ab", "1"};
  EXPECT_TRUE(args.matchers.empty());
  EXPECT_TRUE(args.inputFiles.empty());
  EXPECT_EQ(matcherArgs, bmatcherArgs);
}

TEST(CommandLineArgsTest, ValidateCommandLineArgs_InputFilesWithMatchers) {
  // test --components flag
  std::string errmsg;
  constexpr int argc = 6;
  // args: clang_xform --input-files f1 f2 -m RenameFcn \
  // --matcher-args-RenameFcn --qualified-name Foo --new-name Bar
  const char* argv[argc] = {"clang_xform", "--input-files", "f1", "f2", "-m", "RenameFcn"};
  auto args = ProcessCommandLine(argc, const_cast<char**>(argv));
  std::vector<std::string> matcherArgs = {"--matcher-args-RenameFcn", "--qualified-name", "Foo",
                                          "--new-name", "Bar"};
  EXPECT_TRUE(ValidateCommandLineArgs(args, matcherArgs, false, errmsg));
}

TEST(CommandLineArgsTest, ValidateCommandLineArgs_Apply) {
  // test --apply flag
  std::string errmsg;
  constexpr int argc = 3;
  // args: clang_xform -a output.yaml
  const char* argv[argc] = {"clang_xform", "-a", "output.yaml"};
  auto args = ProcessCommandLine(argc, const_cast<char**>(argv));
  EXPECT_TRUE(ValidateCommandLineArgs(args, std::vector<std::string>(), false, errmsg));
}

TEST(CommandLineArgsTest, ValidateCommandLineArgs_Display) {
  // test --display flag
  std::string errmsg;
  constexpr int argc = 2;
  // args: clang_xform -d
  const char* argv[argc] = {"clang_xform", "-d"};
  auto args = ProcessCommandLine(argc, const_cast<char**>(argv));
  EXPECT_TRUE(ValidateCommandLineArgs(args, std::vector<std::string>(), false, errmsg));
}

TEST(CommandLineArgsTest, ValidateCommandLineArgs_CompDBWithClangFlags) {
  // test mutually exclusive flags between --compile-commands and clang flags
  std::string errmsg;
  constexpr int argc = 3;
  // args: clang_xform --compile-commands compdb.json --
  const char* argv[argc] = {"clang_xform", "--compile-commands", "compdb.json"};
  auto args = ProcessCommandLine(argc, const_cast<char**>(argv));
  EXPECT_FALSE(ValidateCommandLineArgs(args, std::vector<std::string>(), true, errmsg));
}

TEST(CommandLineArgsTest, ValidateCommandLineArgs_DisplayWithOtherFlags) {
  // test mutually exclusive flags between --display and all the others
  std::string errmsg;
  constexpr int argc = 4;
  // args: clang_xform --display --input-files f1
  const char* argv[argc] = {"clang_xform", "--display", "--input-files", "f1"};
  auto args = ProcessCommandLine(argc, const_cast<char**>(argv));
  EXPECT_FALSE(ValidateCommandLineArgs(args, std::vector<std::string>(), false, errmsg));
}

TEST(CommandLineArgsTest, ValidateCommandLineArgs_ApplyWithOtherFlags) {
  // test mutually exclusive flags between --apply and all the others
  std::string errmsg;
  constexpr int argc = 4;
  // args: clang_xform --apply output.yaml -d
  const char* argv[argc] = {"clang_xform", "--apply", "output.yaml", "-d"};
  auto args = ProcessCommandLine(argc, const_cast<char**>(argv));
  EXPECT_FALSE(ValidateCommandLineArgs(args, std::vector<std::string>(), false, errmsg));
}

TEST(CommandLineArgsTest, ValidateCommandLineArgs_InvalidExtForApplyFile) {
  // error out if the applied file extension is not .yaml
  std::string errmsg;
  constexpr int argc = 3;
  // args: clang_xform -a output.xml
  const char* argv[argc] = {"clang_xform", "-a", "output.xml"};
  auto args = ProcessCommandLine(argc, const_cast<char**>(argv));
  EXPECT_FALSE(ValidateCommandLineArgs(args, std::vector<std::string>(), false, errmsg));
}

TEST(CommandLineArgsTest, ValidateCommandLineArgs_InvalidExtForOutputFile) {
  // error out if the output file extension is not .yaml
  std::string errmsg;
  constexpr int argc = 5;
  // args: clang_xform --output output --input-files f1
  const char* argv[argc] = {"clang_xform", "--output", "output", "--input-files", "f1"};
  auto args = ProcessCommandLine(argc, const_cast<char**>(argv));
  EXPECT_FALSE(ValidateCommandLineArgs(args, std::vector<std::string>(), false, errmsg));
}

TEST(CommandLineArgsTest, ValidateCommandLineArgs_UnregisteredMatcher) {
  std::string errmsg;
  // error out if the selected matcher is unregisted
  // args: clang_xform --input-files f1 --matchers m1
  constexpr int argc = 5;
  const char* argv[argc] = {"clang_xform", "--input-files", "f1", "--matchers", "m1"};
  auto args = ProcessCommandLine(argc, const_cast<char**>(argv));
  EXPECT_FALSE(ValidateCommandLineArgs(args, std::vector<std::string>(), false, errmsg));
}

TEST(CommandLineArgsTest, ValidateCommandLineArgs_InvalidMatcherArgs) {
  std::string errmsg;
  constexpr int argc = 5;
  // error out when arguments are provided for a non-selected matcher Foo
  // args: clang_xform --input-files f --matchers RenameFcn --matcher-args-Foo --abc 1
  const char* argv[argc] = {"clang_xform", "--input-files", "f", "--matchers", "RenameFcn"};
  auto args = ProcessCommandLine(argc, const_cast<char**>(argv));
  std::vector<std::string> matcherArgs = {"--matcher-args-Foo", "--abc", "1"};
  EXPECT_FALSE(ValidateCommandLineArgs(args, matcherArgs, false, errmsg));
}

TEST(CommandLineArgsTest, ValidateCommandLineArgs_NoMatcher) {
  std::string errmsg;
  constexpr int argc = 3;
  // error out if no matcher is selected to run over files or components
  // args: clang_xform --input-files f
  const char* argv[argc] = {"clang_xform", "--input-files", "f"};
  auto args = ProcessCommandLine(argc, const_cast<char**>(argv));
  EXPECT_FALSE(ValidateCommandLineArgs(args, std::vector<std::string>(), false, errmsg));
}
