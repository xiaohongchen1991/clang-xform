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
  std::vector<const char*> strippedArgs = StripMatcherArgs(argc, argv);
  // After stripping, args become: test -a 2
  // argc should be 3
  EXPECT_EQ(argc, 3);
  constexpr int strippedArgc = 5;
  const char* baseline[strippedArgc] = {"--matcher-args-m1", "--abc", "1", "--matcher-args-m2", "-a"};
  ASSERT_EQ(strippedArgs.size(), strippedArgc);
  for (int i = 0; i < strippedArgc; ++i) {
    EXPECT_TRUE(!strcmp(strippedArgs[i], baseline[i]));
  }
}

TEST(CommandLineArgsUtilTest, StripMatcherArgs_Empty) {
  // args: test -a 2 --bb 3
  int argc = 5;
  const char* argv[] = {"test", "-a", "2", "--bb", "3"};
  std::vector<const char*> strippedArgs = StripMatcherArgs(argc, argv);
  // After stripping, args remainds the same
  // argc should be 5
  EXPECT_EQ(argc, 5);
  EXPECT_TRUE(strippedArgs.empty());
}

TEST(CommandLineArgsUtilTest, GetMatcherArgs) {
  // args: --matcher-args-m1 --abc 1 --matcher-args-m2 -a
  int argc = 5;
  std::vector<const char*> args = {"--matcher-args-m1", "--abc", "1", "--matcher-args-m2", "-a"};
  std::string matcherID = "m1";
  std::vector<const char*> matcherArgs = GetMatcherArgs(args, matcherID);
  std::vector<const char*> baseline = {"--matcher-args-m1", "--abc", "1"};
  ASSERT_EQ(matcherArgs.size(), baseline.size());
  for (int i = 0; i < baseline.size(); ++i) {
    EXPECT_TRUE(!strcmp(matcherArgs[i], baseline[i]));
  }
  std::vector<const char*> emptyArgs = GetMatcherArgs(args, "m3");
  EXPECT_TRUE(emptyArgs.empty());
}
