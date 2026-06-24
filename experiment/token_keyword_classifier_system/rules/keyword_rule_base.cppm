import <string_view>;

export module keyword_rule_base_module;

export template<typename Derived>
struct KeywordRuleBase {
    inline bool matches(const std::string_view str) {
        return static_cast<Derived*>(this)->matches_impl(str);
    }
};