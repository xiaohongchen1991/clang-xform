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

#ifndef MATCHER_FACTORY_HPP
#define MATCHER_FACTORY_HPP

#include<string>
#include<memory>
#include<map>
#include<vector>

#include <boost/core/noncopyable.hpp>

#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Tooling/Core/Replacement.h"

class MatcherFactory : private boost::noncopyable {
  public:
    typedef clang::ast_matchers::MatchFinder::MatchCallback MatchCallback;
    typedef clang::ast_matchers::internal::DynTypedMatcher DynTypedMatcher;
    typedef std::unique_ptr<MatchCallback> (*CreateCallbackFunction)(clang::tooling::Replacements&);
    
    static MatcherFactory& Instance() {
        static MatcherFactory factory;
        return factory;
    }
    
    void RegisterMatcher(const std::string& id,
                         const std::vector<DynTypedMatcher>& matchers,
                         CreateCallbackFunction fcn) {
        matcher_map_.emplace(id, std::make_pair(matchers, fcn));
    }
    
    std::unique_ptr<MatchCallback>
    CreateCallback(const std::string& id, clang::tooling::Replacements& replacements) const {
        auto iter = matcher_map_.find(id);
        if (iter == matcher_map_.end()) {
            return nullptr;
        } else {
            return (iter->second.second)(replacements);
        }
    }

    const std::vector<DynTypedMatcher>*
    CreateMatchers(const std::string& id)  const {
        auto iter = matcher_map_.find(id);
        if (iter == matcher_map_.end()) {
            return nullptr;
        } else {
            return &(iter->second.first);
        }
    }

    const std::map<std::string, std::pair<std::vector<DynTypedMatcher>, CreateCallbackFunction> >&
    getMatcherMap() const {
        return matcher_map_;
    }

  private:
    std::map<std::string, std::pair<std::vector<DynTypedMatcher>, CreateCallbackFunction> > matcher_map_;
};

template <class Callback>
class MatcherHelper {
  public:
    typedef clang::ast_matchers::MatchFinder::MatchCallback MatchCallback;
    typedef clang::ast_matchers::internal::DynTypedMatcher DynTypedMatcher;
    
    MatcherHelper(const std::string& id, const std::vector<DynTypedMatcher>& matchers) {
        MatcherFactory& factory = MatcherFactory::Instance();
        factory.RegisterMatcher(id, matchers, MatcherHelper<Callback>::CreateCallback);
    }
    static std::unique_ptr<MatchCallback> CreateCallback(clang::tooling::Replacements& replacements) {
        return std::make_unique<Callback>(replacements);
    }
};

#endif
