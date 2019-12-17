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

#include <fstream>
#include <cstdio>

#include "gtest/gtest.h"

// fixture class for CompareFilesTest suite
class CompareFilesTest : public ::testing::Test {
 protected:
  std::string fileName1;
  std::string fileName2;

  void SetUp() override {
    fileName1 = "tmp1.txt";
    fileName2 = "tmp2.txt";
  }

  void TearDown() override {
    remove(fileName1.c_str());
    remove(fileName2.c_str());
  }
};

// fixture class for IsEmptyFileTest suite
class IsEmptyFileTest : public ::testing::Test {
 protected:
  std::string fileName;

  void SetUp() override {
    fileName = "tmp.txt";
  }

  void TearDown() override {
    remove(fileName.c_str());
  }
};

TEST_F(CompareFilesTest, SameFiles) {
  // setup
  std::ofstream ofs(fileName1);
  ofs << "Foo\n";
  ofs.close();

  ofs.open(fileName2);
  ofs << "Foo\n";
  ofs.close();

  // expect same files
  EXPECT_TRUE(CompareFiles(fileName1, fileName2));
}

TEST_F(CompareFilesTest, DifferentFiles) {
  // setup
  std::ofstream ofs(fileName1);
  ofs << "Foo\n";
  ofs.close();

  ofs.open(fileName2);
  ofs << "Bar\n";
  ofs.close();

  // expect different files
  EXPECT_FALSE(CompareFiles(fileName1, fileName2));
}

TEST_F(IsEmptyFileTest, Empty) {
  // setup an empty file
  std::ofstream ofs(fileName);
  ofs.close();

  // expect empty file
  EXPECT_TRUE(IsEmptyFile(fileName));
}

TEST_F(IsEmptyFileTest, NonEmpty) {
  // setup a non-empty file
  std::ofstream ofs(fileName);
  ofs << "Foo\n";
  ofs.close();

  // expect a non-empty file
  EXPECT_FALSE(IsEmptyFile(fileName));
}
