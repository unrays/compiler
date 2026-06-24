import dfa_lexer;
export module lexical_analyser_module;
import lexing_state_type_module;

#pragma once
import <string>;
import <vector>;
import <ranges>;
import <iostream>;
//#include "../core/macros.hpp"
//#include "../core/Concepts/is_lexing_automaton.hpp"
//#include "../core/Concepts/is_token_classifier.hpp"
//#include "helpers/logging.hpp"
//#include "helpers/state_to_token_conversion.hpp"
//
//#include ""

//#include "lexing_state_type.cppm"

import token_module;


// e26-unrays/Projet/include/Core/concepts/is_token_classifier.hpp
/*************************************************************/

#if 0
template<typename T>
concept ITokenClassifier = std::derived_from<T, TokenClassifierBase<T>>;

template<typename... Classifiers>
concept AllTokenClassifiers =
(ITokenClassifier<Classifiers> && ...);
#endif


// e26-unrays/Projet/include/Core/concepts/is_lexing_automaton.hpp
/*************************************************************/

#if 0
template<typename T>
concept ILexingAutomaton = std::derived_from<T, LexingAutomatonBase<T>>;
#endif


// e26-unrays/Projet/include/Lexer/Helpers/Logging.hpp
/*************************************************************/

constexpr std::string_view tokentype_to_string(TokenType t) {
    switch (t) {
    case TokenType::Identifier: return "Identifier";

    case TokenType::Keyword: return "Keyword";
    case TokenType::Kwrd_Type: return "Kwrd_Type";
    case TokenType::Kwrd_Qualifier: return "Kwrd_Qualifier";
    case TokenType::Kwrd_Specifier: return "Kwrd_Specifier";
    case TokenType::Kwrd_Modifier: return "Kwrd_Modifier";
    case TokenType::Kwrd_Alignment: return "Kwrd_Alignment";
    case TokenType::Kwrd_Control: return "Kwrd_Control";
    case TokenType::Kwrd_Access: return "Kwrd_Access";

    case TokenType::Delimiter: return "Delimiter";
    case TokenType::Delim_Colon: return "Delim_Colon";
    case TokenType::Delim_Semicolon: return "Delim_Semicolon";
    case TokenType::Delim_Coma: return "Delim_Coma";
    case TokenType::Delim_LParen: return "Delim_LParen";
    case TokenType::Delim_RParen: return "Delim_RParen";
    case TokenType::Delim_LCurly: return "Delim_LCurly";
    case TokenType::Delim_RCurly: return "Delim_RCurly";
    case TokenType::Delim_LSquare: return "Delim_LSquare";
    case TokenType::Delim_RSquare: return "Delim_RSquare";
    case TokenType::Delim_LAngle: return "Delim_LAngle";
    case TokenType::Delim_RAngle: return "Delim_RAngle";

    case TokenType::Preprocessor: return "Preprocessor";
    case TokenType::Operator: return "Operator";
    case TokenType::Number: return "Number";
    case TokenType::Whitespace: return "Whitespace";
    case TokenType::Newline: return "Newline";
    case TokenType::Invalid: return "Invalid";
    case TokenType::Unknown: return "Unknown";
    }

    return "UNABLE TO CONVERT TOKENTYPE TO STRING";
}

/*************************************************************/

constexpr TokenType state_to_token(LexingStateType _) {
    switch (_) {
    case LexingStateType::STATE_IDENTIFIER: return TokenType::Identifier;
    case LexingStateType::STATE_DELIMITER: return TokenType::Delimiter;

    case LexingStateType::STATE_OPERATOR: return TokenType::Operator;

    case LexingStateType::STATE_DELIM_COLON: return TokenType::Delim_Colon;
    case LexingStateType::STATE_DELIM_SEMI: return TokenType::Delim_Semicolon;
    case LexingStateType::STATE_DELIM_COMA: return TokenType::Delim_Coma;

    case LexingStateType::STATE_DELIM_L_PAREN: return TokenType::Delim_LParen;
    case LexingStateType::STATE_DELIM_R_PAREN: return TokenType::Delim_RParen;

    case LexingStateType::STATE_DELIM_L_CURLY: return TokenType::Delim_LCurly;
    case LexingStateType::STATE_DELIM_R_CURLY: return TokenType::Delim_RCurly;

    case LexingStateType::STATE_DELIM_L_SQUARE: return TokenType::Delim_LSquare;
    case LexingStateType::STATE_DELIM_R_SQUARE: return TokenType::Delim_RSquare;

    case LexingStateType::STATE_DELIM_L_ANGLE: return TokenType::Delim_LAngle;
    case LexingStateType::STATE_DELIM_R_ANGLE: return TokenType::Delim_RAngle;

    case LexingStateType::STATE_PREPROCESSOR: return TokenType::Preprocessor;
    case LexingStateType::STATE_NEWLINE: return TokenType::Newline;

    case LexingStateType::STATE_NUMBER: return TokenType::Number;
    case LexingStateType::STATE_INVALID: return TokenType::Unknown;

    default: return TokenType::Invalid;
    }
}

/*************************************************************/

//template<
//    ILexingAutomaton Automaton,
//    AllTokenClassifiers... Classifiers
//>

export template< // on retire les modèles afin de tester, à refactoriser prochainement
    typename Automaton,
    typename... Classifiers
>
struct LexicalAnalyzer {
public:
    std::vector<Token> tokenize(const std::vector<std::string> split) {
        std::string buffer{}; std::vector<Token> tokens;
        LexingStateType current_state{};

        for (auto const& s : split) {
            for (std::size_t cursor = 0; cursor < s.size();) {
                auto c = s[cursor];

                auto previous_state = std::move(current_state);
                current_state = automaton.step(current_state, c);

                if (current_state == LexingStateType::STATE_INVALID) {
                    tokens.push_back({
                        state_to_token(previous_state),
                        std::move(buffer)
                        });

                    current_state = LexingStateType::STATE_START;
                }
                else {
                    buffer.push_back(c);
                    ++cursor;
                }
            }

            if (current_state != LexingStateType::STATE_INVALID)
                tokens.push_back({
                    state_to_token(current_state),
                    std::move(buffer)
                    });

            current_state = LexingStateType::STATE_START;
        }

        for (auto& t : tokens | std::ranges::views::filter(
            [](auto& cur) {
                return cur.type == TokenType::Identifier ||
                    cur.type == TokenType::Operator;
            }))
            std::apply([&](auto&& classifier) {
            classifier.transform(t);
                }, classifiers);

        for (auto& t : tokens) {
            std::cout << "Type: " << tokentype_to_string(t.type)
                << ", content: " << t.lexeme << "\n";
        }

        return tokens;
    }

private:
    std::tuple<Classifiers...> classifiers;
    Automaton automaton{};
};