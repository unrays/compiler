#pragma once
import keyword_rule_base_module;
import <string_view>;

//import keyword_rule_base_module;

struct TypeKeywordRule :
    public KeywordRuleBase<TypeKeywordRule> {
    friend KeywordRuleBase<TypeKeywordRule>;
private:
    inline bool matches_impl(const std::string_view str) const {
        return str == "bool"
            || str == "char"
            || str == "char8_t"
            || str == "char16_t"
            || str == "char32_t"
            || str == "double"
            || str == "float"
            || str == "int"
            || str == "long"
            || str == "short"
            || str == "signed"
            || str == "unsigned"
            || str == "void"
            || str == "wchar_t";
    }
};