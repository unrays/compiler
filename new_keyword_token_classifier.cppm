import <iostream>;
import <string_view>;
import <string>;

export module new_keyword_classifier_module;

import token_module;

/********************************************************************************/

// dans le fond, juste retirer le tuple et tout garder en static

/********************************************************************************/

template <std::size_t N>
struct consteval_string {
    char value[N]{};

    consteval consteval_string(const char(&str)[N]) noexcept {
        std::copy_n(str, N, value);
    }

    constexpr std::string_view view() const noexcept {
        return std::string_view(value, N - 1);
    }

    //constexpr std::string as_string() const { // copie
    //    return std::string(value, N - 1);
    //}

    constexpr operator std::string_view() const noexcept {
        return std::string_view(value, N - 1);
    }

    inline operator std::string() const { // copie
        return std::string(value, N - 1);
    }
};

/********************************************************************************/

template<typename T>
struct KeywordRuleBase {};

/********************************************************************************/

struct SpecifierKeywordRule :
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

struct SpecifierKeywordMatcher {
    static constexpr bool matches(std::string_view sv) noexcept {

    }
};

/********************************************************************************/

template<consteval_string... Pinciples>
struct KeywordMatchingPolicy { // ou KeywordValidationPolicy

    static constexpr bool matches(std::string_view sv) noexcept {
        return ((sv == Pinciples) || ...);
    }
};


/********************************************************************************/

#if 0
export template<AllLexingContexts... Contexts>
struct TokenKeywordClassifier {
public: // était crtp mais on refactorisera plus tard, pour l'instant, je le retire

    Token& transform(Token& token) const {
        auto& [type, content] = token;
        bool matched = false;

        (([&] {
            decltype(auto) rule = typename Contexts::Rule{}; //ici refactoriser, perf
            if (rule.matches(content)) {
                type = Contexts::Target;
                matched = true;
            }
            }(), !matched) && ...);

        return token;
    }

    //    typename T::Rule;
    //    T::Target;

private:
    static constexpr auto lexing_contexts = std::tuple{
        std::tuple<typename Contexts::Rule>{}...
    };
};
#endif

/********************************************************************************/

using AccessKeywordMatchingPolicy = KeywordMatchingPolicy<
    "public", "protected", "private"
>;

using AlignmentKeywordMatchingPolicy = KeywordMatchingPolicy<
    "alignas", "alignof"
>;

using ControlKeywordMatchingPolicy = KeywordMatchingPolicy<
    "and", "and_eq", "asm", "atomic_cancel",
    "atomic_commit", "atomic_noexcept", "break", "case",
    "catch", "class", "compl", "concept",
    "continue", "co_await", "co_return", "co_yield",
    "default", "delete", "do", "else",
    "enum", "false", "for", "goto",
    "if", "namespace", "new", "not",
    "not_eq", "nullptr", "operator", "or",
    "or_eq", "reflexpr", "requires", "return",
    "sizeof", "switch", "synchronized", "template",
    "this", "throw", "true", "try",
    "typedef", "typeid", "typename", "union",
    "using", "while", "xor", "xor_eq"
>;

using ModifierKeywordMatchingPolicy = KeywordMatchingPolicy<
    "*", "&", "&&"
>;

using QualifierKeywordMatchingPolicy = KeywordMatchingPolicy<
    "const", "volatile"
>;

using SpecifierKeywordMatchingPolicy = KeywordMatchingPolicy<
    "virtual", "final", "override",
    "constexpr", "consteval", "constinit",
    "inline", "explicit", "noexcept",
    "static", "extern", "thread_local",
    "mutable", "register", "export"
>;

using TypeKeywordMatchingPolicy = KeywordMatchingPolicy<
    "bool", "char", "char8_t", "char16_t",
    "char32_t", "double", "float", "int",
    "long", "short", "signed", "unsigned",
    "void", "wchar_t"
>;

/********************************************************************************/
//
//export template<typename, auto>
//struct LexingContext_old;
//
//template<
//    IKeywordRule KRule,
//    TokenType Token
//>
//struct LexingContext_old<KRule, Token> {
//    using Rule = KRule;
//    static constexpr TokenType Target = Token;
//};

/********************************************************************************/

template<typename T>
concept IsaKeywordMatchingPolicy =
    requires(std::string_view sv) {
        { T::matches(sv) } -> std::same_as<bool>;
};

/********************************************************************************/

export template<typename Predicate, auto Corresponding>
struct TokenClassifierContext final {
    using predicate_type = Predicate;
    static constexpr TokenType corresponding = Corresponding;
};

template<typename T>
concept is_token_classifier_context = requires {
    typename T::predicate_type;
    T::corresponding;
};

/********************************************************************************/

export template<is_token_classifier_context... Contexts>
struct TokenKeywordClassifier2 final { // re-categorizer?
private:
    template<typename Current, typename... Remaining>
    [[nodiscard]] static constexpr TokenType evaluate_recursively(std::string_view sv)
        noexcept(noexcept(Current::predicate_type::matches(sv)))
    {
        if (Current::predicate_type::matches(sv))
            return Current::corresponding;

        if constexpr (sizeof...(Remaining) == 0)
            return TokenType::Unknown;
        else
            return evaluate_recursively<Remaining...>(sv);
    }

public:
    [[nodiscard]] static constexpr TokenType transform(std::string_view sv)
        noexcept(noexcept(evaluate_recursively<Contexts...>(sv)))
    {
        return evaluate_recursively<Contexts...>(sv);
    }
};


/********************************************************************************/


export void kwrd_classifier_main() {


    bool res = SpecifierKeywordMatchingPolicy::matches("constexprr");

    std::cout << res << "\n";


    using TokenKwrdClassifier = TokenKeywordClassifier2<
        TokenClassifierContext<AccessKeywordMatchingPolicy,    TokenType::Kwrd_Access>,
        TokenClassifierContext<AlignmentKeywordMatchingPolicy, TokenType::Kwrd_Alignment>
    >;

    auto res2 = TokenKwrdClassifier::transform("public");

    std::cout << static_cast<int>(res2) << "\n";



}