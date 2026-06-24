#pragma once

import keyword_rule_base_module;
import <string_view>;

//import keyword_rule_base_module;

struct ModifierKeywordRule:
    public KeywordRuleBase<ModifierKeywordRule> {
    friend KeywordRuleBase<ModifierKeywordRule>;
private:
    inline bool matches_impl(const std::string_view str) const {
        return str == "*" || str == "&" || str == "&&";
    }
};