
export module token_keyword_classifier_module;

import token_module;

import <concepts>;
import <tuple>;
//#include "rules/access_keyword_rule.cppm"
//#include "rules/alignment_keyword_rule.cppm"




//#include "rules/control_keyword_rule.cppm"
//#include "rules/modifier_keyword_rule.cppm"
//#include "rules/qualifier_keyword_rule.cppm"
//#include "rules/specifier_keyword_rule.cppm"
//#include "rules/type_keyword_rule.cppm"


//#include "rules/keyword_rule_base.cppm"
import keyword_rule_base_module;

//export module keyword_rule_base_module;

// e26-unrays/Projet/include/Core/concepts/is_lexing_context.hpp
/*************************************************************/

export template<typename T>
concept IsLexingContext = requires {
    typename T::Rule;
    T::Target;
};

export template<typename... Contexts>
concept AllLexingContexts =
(IsLexingContext<Contexts> && ...);


//e26-unrays/Projet/include/Core/concepts/is_keyword_rule.hpp
/*************************************************************/

export template<typename T>
concept IKeywordRule = std::derived_from<T, KeywordRuleBase<T>>;


// e26-unrays/Projet/include/Core/lexing_context.hpp
/*************************************************************/


export template<typename, auto>
struct LexingContext_old;

template<
    IKeywordRule KRule,
    TokenType Token
>
struct LexingContext_old<KRule, Token> {
    using Rule = KRule;
    static constexpr TokenType Target = Token;
};

/*************************************************************/

// GENRE IL FAUT COMPLÈTEMENT DE ZÉRO, POUR L'INSTANT, IL EST FONCTIONNEL

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