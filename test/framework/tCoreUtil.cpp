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

#include "CoreUtil.hpp"

#include <fstream>
#include <cstdio>

#include "gtest/gtest.h"

TEST(CoreUtilTest, ParseConfigFile_EqualityAsKeyDelimiter) {
  std::string configFile = "tmp.cfg";
  std::ofstream ofs(configFile);
  ofs << "Matchers= m1\n"
      << "Matchers=m2\n"
      << "Matchers =m3";
  ofs.close();
  auto res = ParseConfigFile(configFile, "Matchers");
  // remove tmp config file
  remove(configFile.c_str());
  std::vector<std::string> baseline = {"m1", "m2", "m3"};
  EXPECT_EQ(res, baseline);
}

TEST(CoreUtilTest, ParseConfigFile_SpaceAsKeyDelimiter) {
  std::string configFile = "tmp.cfg";
  std::ofstream ofs(configFile);
  ofs << "Matchers m1\n"
      << "Matchers  m2";
  ofs.close();
  auto res = ParseConfigFile(configFile, "Matchers");
  // remove tmp config file
  remove(configFile.c_str());
  std::vector<std::string> baseline = {"m1", "m2"};
  EXPECT_EQ(res, baseline);
}

TEST(CoreUtilTest, ParseConfigFile_MixedKeyDelimiter) {
  std::string configFile = "tmp.cfg";
  std::ofstream ofs(configFile);
  ofs << "Matchers = m1\n"
      << "Matchers m2";
  ofs.close();
  auto res = ParseConfigFile(configFile, "Matchers");
  // remove tmp config file
  remove(configFile.c_str());
  std::vector<std::string> baseline = {"m1", "m2"};
  EXPECT_EQ(res, baseline);
}

TEST(CoreUtilTest, ParseConfigFile_SpaceAsValueDelimiter) {
  std::string configFile = "tmp.cfg";
  std::ofstream ofs(configFile);
  ofs << "Matchers = m1  m2\n"
      << "Matchers m3 m4";
  ofs.close();
  auto res = ParseConfigFile(configFile, "Matchers");
  // remove tmp config file
  remove(configFile.c_str());
  std::vector<std::string> baseline = {"m1", "m2", "m3", "m4"};
  EXPECT_EQ(res, baseline);
}

TEST(CoreUtilTest, ParseConfigFile_CommaAsValueDelimiter) {
  std::string configFile = "tmp.cfg";
  std::ofstream ofs(configFile);
  ofs << "Matchers = m1, m2\n"
      << "Matchers m3,m4";
  ofs.close();
  auto res = ParseConfigFile(configFile, "Matchers");
  // remove tmp config file
  remove(configFile.c_str());
  std::vector<std::string> baseline = {"m1", "m2", "m3", "m4"};
  EXPECT_EQ(res, baseline);
}

TEST(CoreUtilTest, ParseConfigFile_MixedValueDelimiter) {
  std::string configFile = "tmp.cfg";
  std::ofstream ofs(configFile);
  ofs << "Matchers = m1, m2\n"
      << "Matchers m3 m4";
  ofs.close();
  auto res = ParseConfigFile(configFile, "Matchers");
  // remove tmp config file
  remove(configFile.c_str());
  std::vector<std::string> baseline = {"m1", "m2", "m3", "m4"};
  EXPECT_EQ(res, baseline);
}

TEST(CoreUtilTest, ParseConfigFileForMatcherArgs) {
  std::string configFile = "tmp.cfg";
  std::ofstream ofs(configFile);
  ofs << "matcher-args-m1 --ab 1 --cd 2\n"
      << "matcher-args-m2 --ab=1 --cd=2";
  ofs.close();
  auto res = ParseConfigFileForMatcherArgs(configFile);
  // remove tmp config file
  remove(configFile.c_str());
  std::vector<std::string> baseline = {"--matcher-args-m1", "--ab", "1", "--cd", "2",
                                       "--matcher-args-m2", "--ab=1","--cd=2"};
  EXPECT_EQ(res, baseline);
}
