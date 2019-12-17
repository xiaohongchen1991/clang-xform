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

#ifndef MATCH_CALLBACK_DEF_HPP
#define MATCH_CALLBACK_DEF_HPP

#include "MatchCallbackBase.hpp"

#define MATCH_CALLBACK(NAME)                                                                \
class NAME : public MatchCallbackBase {                                                     \
  public :                                                                                  \
    explicit NAME (const std::string& id,                                                   \
                   clang::tooling::Replacements& replacements,                              \
                   std::vector<std::string> args)                                           \
        : MatchCallbackBase(id, replacements, std::move(args))                              \
    {}                                                                                      \
    virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) override; \
    virtual void RegisterMatchers(clang::ast_matchers::MatchFinder* finder) override;       \
}

#define OPTION_MATCH_CALLBACK(NAME)                                                         \
class NAME : public MatchCallbackBase {                                                     \
  public :                                                                                  \
    explicit NAME (const std::string& id,                                                   \
                   clang::tooling::Replacements& replacements,                              \
                   std::vector<std::string> args)                                           \
        : MatchCallbackBase(id, replacements, std::move(args))                              \
    {}                                                                                      \
    virtual void run(const clang::ast_matchers::MatchFinder::MatchResult &Result) override; \
    virtual void RegisterMatchers(clang::ast_matchers::MatchFinder* finder) override;       \
    virtual void RegisterOptions() override;                                                \
}


#endif
