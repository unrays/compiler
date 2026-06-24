#pragma once
//#include "keyword_rule_base.cppm"
import <string_view>;

import keyword_rule_base_module;

struct AccessKeywordRule :
    public KeywordRuleBase<AccessKeywordRule> {
    friend KeywordRuleBase<AccessKeywordRule>;
private:
    inline bool matches_impl(const std::string_view str) const {
        return str == "public"
            || str == "protected"
            || str == "private";
    }
};