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

#include "CommandLineArgsUtil.hpp"

#include "gtest/gtest.h"

TEST(CommandLineArgsUtilTest, StripMatcherArgs) {
  // args: test -a 2 --matcher-args-m1 --abc 1 --matcher-args-m2 -a
  int argc = 8;
  const char* argv[] = {"test", "-a", "2", "--matcher-args-m1", "--abc",
                        "1", "--matcher-args-m2", "-a"};
  std::vector<std::string> strippedArgs = StripMatcherArgs(argc, argv);
  // After stripping, args become: test -a 2
  // argc should be 3
  EXPECT_EQ(argc, 3);
  std::vector<std::string> baseline = {"--matcher-args-m1", "--abc", "1", "--matcher-args-m2", "-a"};
  EXPECT_EQ(strippedArgs, baseline);
}

TEST(CommandLineArgsUtilTest, StripMatcherArgs_Empty) {
  // args: test -a 2 --bb 3
  int argc = 5;
  const char* argv[] = {"test", "-a", "2", "--bb", "3"};
  std::vector<std::string> strippedArgs = StripMatcherArgs(argc, argv);
  // After stripping, args remainds the same
  // argc should be 5
  EXPECT_EQ(argc, 5);
  EXPECT_TRUE(strippedArgs.empty());
}

TEST(CommandLineArgsUtilTest, GetMatcherArgs) {
  // args: --matcher-args-m1 --abc 1 --matcher-args-m2 -a
  int argc = 5;
  std::vector<std::string> args = {"--matcher-args-m1", "--abc", "1", "--matcher-args-m2", "-a"};
  std::string matcherID = "m1";
  std::vector<std::vector<std::string> > matcherArgs = GetMatcherArgs(args, matcherID);
  std::vector<std::string> baseline = {"--matcher-args-m1", "--abc", "1"};
  ASSERT_EQ(matcherArgs.size(), 1);
  EXPECT_EQ(matcherArgs[0], baseline);
  std::vector<std::vector<std::string> > emptyArgs = GetMatcherArgs(args, "m3");
  EXPECT_TRUE(emptyArgs.empty());
}

TEST(CommandLineArgsUtilTest, GetMatcherArgs_MultipleConfig) {
  // args: --matcher-args-m --abc 1 --matcher-args-m --ab 2
  int argc = 6;
  std::vector<std::string> args = {"--matcher-args-m", "--abc", "1", "--matcher-args-m", "--ab", "2"};
  std::string matcherID = "m";
  std::vector<std::vector<std::string> > matcherArgs = GetMatcherArgs(args, matcherID);
  std::vector<std::string> baseline1 = {"--matcher-args-m", "--abc", "1"};
  std::vector<std::string> baseline2 = {"--matcher-args-m", "--ab", "2"};
  ASSERT_EQ(matcherArgs.size(), 2);
  EXPECT_EQ(matcherArgs[0], baseline1);
  EXPECT_EQ(matcherArgs[1], baseline2);
}
