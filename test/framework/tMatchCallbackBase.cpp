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
#include "MockMatchCallback.hpp"

#include "gtest/gtest.h"

namespace {

// derived test class from MatchCallbackBase
class MatchCallbackForTest : public MatchCallbackBase {
 public:
  explicit MatchCallbackForTest(const std::string& id,
                                clang::tooling::Replacements& replacements,
                                std::vector<const char*> args)
      : MatchCallbackBase(id, replacements, std::move(args))
  {}

  void RegisterMatchers(clang::ast_matchers::MatchFinder* finder) override {}
  void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) override {}

  template <typename T>
  void AddOption(const std::string& key) {
    MatchCallbackBase::AddOption<T>(key);
  }

  template <typename T>
  void AddOption(const std::string& key, T default_value) {
    MatchCallbackBase::AddOption<T>(key, default_value);
  }

  template <typename T>
  T GetOption(const std::string& key) {
    return MatchCallbackBase::GetOption<T>(key);
  }

};

} // end of anonymous namespace

// fixture class for MatchCallbackBase suite
class MatchCallbackBaseTest : public ::testing::Test {
 protected:
  clang::tooling::Replacements replacements;
  std::string matcherName;
  std::string key1;
  std::string key2;
  std::string value1;
  std::string value2;
  std::string sep;

  void SetUp() override {
    matcherName = "Test";
    key1 = "key1";
    key2 = "key2";
    value1 = "value1";
    value2 = "value2";
    sep = "--matcher-args-" + matcherName;
  }
};

TEST_F(MatchCallbackBaseTest, AddAndGetOption) {
  // "--matcher-args-Test --key1 value1 --key2 value2"
  std::vector<const char*> args(5);
  std::string keyArg1 = "--" + key1;
  std::string keyArg2 = "--" + key2;
  args[0] = sep.c_str();
  args[1] = keyArg1.c_str();
  args[2] = value1.c_str();
  args[3] = keyArg2.c_str();
  args[4] = value2.c_str();
  MatchCallbackForTest matchCallback(matcherName, replacements, args);
  matchCallback.AddOption<std::string>(key1);
  matchCallback.AddOption<std::string>(key2);
  matchCallback.ParseOptions();

  EXPECT_EQ(matchCallback.GetOption<std::string>(key1), value1);
  EXPECT_EQ(matchCallback.GetOption<std::string>(key2), value2);
}

TEST_F(MatchCallbackBaseTest, AddDefaultOption) {
  // "--matcher-args-Test --key1 value1"
  std::vector<const char*> args(3);
  std::string keyArg1 = "--" + key1;
  args[0] = sep.c_str();
  args[1] = keyArg1.c_str();
  args[2] = value1.c_str();
  MatchCallbackForTest matchCallback(matcherName, replacements, args);
  matchCallback.AddOption<std::string>(key1);
  matchCallback.AddOption<std::string>(key2, value2);
  matchCallback.ParseOptions();

  EXPECT_EQ(matchCallback.GetOption<std::string>(key1), value1);
  EXPECT_EQ(matchCallback.GetOption<std::string>(key2), value2);
}

TEST_F(MatchCallbackBaseTest, MissingArgumentSeparator) {
  // "--key1 value1 --key2 value2"
  std::vector<const char*> args(4);
  std::string keyArg1 = "--" + key1;
  std::string keyArg2 = "--" + key2;
  args[0] = keyArg1.c_str();
  args[1] = value1.c_str();
  args[2] = keyArg2.c_str();
  args[3] = value2.c_str();
  EXPECT_THROW(MatchCallbackForTest(matcherName, replacements, args),
               CommandLineOptionException);
}

TEST_F(MatchCallbackBaseTest, OptionNotPresent) {
  // "--matcher-args-Test --key1 value1"
  std::vector<const char*> args(3);
  std::string keyArg1 = "--" + key1;
  args[0] = sep.c_str();
  args[1] = keyArg1.c_str();
  args[2] = value1.c_str();
  MatchCallbackForTest matchCallback(matcherName, replacements, args);
  matchCallback.AddOption<std::string>(key1);
  matchCallback.ParseOptions();

  EXPECT_THROW(matchCallback.GetOption<std::string>(key2),
               cxxopts::option_not_present_exception);
}

TEST_F(MatchCallbackBaseTest, OptionNotExist) {
  // "--matcher-args-Test --key1 value1 --key2 value2"
  std::vector<const char*> args(5);
  std::string keyArg1 = "--" + key1;
  std::string keyArg2 = "--" + key2;
  args[0] = sep.c_str();
  args[1] = keyArg1.c_str();
  args[2] = value1.c_str();
  args[3] = keyArg2.c_str();
  args[4] = value2.c_str();
  MatchCallbackForTest matchCallback(matcherName, replacements, args);
  matchCallback.AddOption<std::string>(key1);
  EXPECT_THROW(matchCallback.ParseOptions(),
               cxxopts::option_not_exists_exception);
}

TEST_F(MatchCallbackBaseTest, RegisterOrder) {
  clang::ast_matchers::MatchFinder* finder = nullptr;
  std::vector<const char*> args;
  MockMatchCallback matchCallback(matcherName, replacements, args);

  {
    ::testing::InSequence seq;
    EXPECT_CALL(matchCallback, RegisterOptions()).Times(1);
    EXPECT_CALL(matchCallback, ParseOptions()).Times(1);
    EXPECT_CALL(matchCallback, RegisterMatchers(finder)).Times(1);
  }

  matchCallback.Register(finder);
}
