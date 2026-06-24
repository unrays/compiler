#pragma once
import keyword_rule_base_module;
import <string_view>;

//import keyword_rule_base_module;

struct ControlKeywordRule :
    public KeywordRuleBase<ControlKeywordRule> {
    friend KeywordRuleBase<ControlKeywordRule>;
private:
    inline bool matches_impl(const std::string_view str) const {
        return str == "and"
            || str == "and_eq"
            || str == "asm"
            || str == "atomic_cancel"
            || str == "atomic_commit"
            || str == "atomic_noexcept"
            || str == "break"
            || str == "case"
            || str == "catch"
            || str == "class"
            || str == "compl"
            || str == "concept"
            || str == "continue"
            || str == "co_await"
            || str == "co_return"
            || str == "co_yield"
            || str == "default"
            || str == "delete"
            || str == "do"
            || str == "else"
            || str == "enum"
            || str == "false"
            || str == "for"
            || str == "goto"
            || str == "if"
            || str == "namespace"
            || str == "new"
            || str == "not"
            || str == "not_eq"
            || str == "nullptr"
            || str == "operator"
            || str == "or"
            || str == "or_eq"
            || str == "reflexpr"
            || str == "requires"
            || str == "return"
            || str == "sizeof"
            || str == "switch"
            || str == "synchronized"
            || str == "template"
            || str == "this"
            || str == "throw"
            || str == "true"
            || str == "try"
            || str == "typedef"
            || str == "typeid"
            || str == "typename"
            || str == "union"
            || str == "using"
            || str == "while"
            || str == "xor"
            || str == "xor_eq";
    }
};