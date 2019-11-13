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

#ifndef MOCK_MATCHCALLBACK_HPP
#define MOCK_MATCHCALLBACK_HPP

#include "MatchCallbackBase.hpp"

#include "gmock/gmock.h"

class MockMatchCallback : public MatchCallbackBase {
 public:
  explicit MockMatchCallback(const std::string& id,
                             clang::tooling::Replacements& replacements,
                             std::vector<const char*> args)
      : MatchCallbackBase(id, replacements, std::move(args))
  {}

  MOCK_METHOD(void, run, (const clang::ast_matchers::MatchFinder::MatchResult &Result), (override));
  MOCK_METHOD(void, RegisterMatchers, (clang::ast_matchers::MatchFinder* finder), (override));
  MOCK_METHOD(void, RegisterOptions, (), (override));
  MOCK_METHOD(void, ParseOptions, (), (override));
};

#endif
