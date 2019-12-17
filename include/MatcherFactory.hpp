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

// forward declaration
class MatchCallbackBase;

namespace clang {
namespace tooling {
class Replacements;
} // end of namespace tooling
} // end of namespace clang

class MatcherFactory {
 public:
  typedef std::unique_ptr<MatchCallbackBase> (*CreateCallbackFunction)(const std::string&,
                                                                       clang::tooling::Replacements&,
                                                                       std::vector<std::string>);
  typedef std::map<std::string, CreateCallbackFunction> MatcherMap;

  MatcherFactory(const MatcherFactory&) = delete;
  MatcherFactory& operator=(const MatcherFactory&) = delete;
    
  static MatcherFactory& Instance() {
    static MatcherFactory factory;
    return factory;
  }
    
  void RegisterMatchCallback(const std::string& id,
                             CreateCallbackFunction fcn) {
    matcher_map_.emplace(id, fcn);
  }
    
  std::unique_ptr<MatchCallbackBase>
  CreateMatchCallback(const std::string& id,
                      clang::tooling::Replacements& replacements,
                      std::vector<std::string> args) const {
    auto iter = matcher_map_.find(id);
    if (iter == matcher_map_.end()) {
      return nullptr;
    } else {
      return (iter->second)(id, replacements, std::move(args));
    }
  }

  const MatcherMap& getMatcherMap() const {
    return matcher_map_;
  }

 private:
  MatcherFactory() = default;
  MatcherMap matcher_map_;
};

#endif
