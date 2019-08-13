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
