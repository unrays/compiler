#pragma once
import keyword_rule_base_module;
import <string_view>;

//import keyword_rule_base_module;

struct SpecifierKeywordRule:
    public KeywordRuleBase<SpecifierKeywordRule> {
    friend KeywordRuleBase<SpecifierKeywordRule>;
private:
    inline bool matches_impl(const std::string_view str) const {
        return str == "virtual" || str == "final" || str == "override" ||
               str == "constexpr" || str == "consteval" || str == "constinit" ||
               str == "inline" || str == "explicit" || str == "noexcept" ||
               str == "static" || str == "extern" || str == "thread_local" ||
               str == "mutable" || str == "register" || str == "export";
    }
};
