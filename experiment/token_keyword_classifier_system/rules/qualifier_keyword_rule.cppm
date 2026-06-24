#pragma once
import keyword_rule_base_module;
import <string_view>;

//import keyword_rule_base_module;

struct QualifierKeywordRule:
    public KeywordRuleBase<QualifierKeywordRule> {
    friend KeywordRuleBase<QualifierKeywordRule>;
private:
    inline bool matches_impl(const std::string_view str) const {
        return str == "const" || str == "volatile";
    }
};