#include <stdexcept>
#include <iostream>
#include <chrono>

export module dfa_lexer;

/***********************************************************************************/

// ce dfa est pour le Token Classifier. Ensuite, c'est le lexer qui utilise le module.
// LexicalAnalyzer<LexingAutomaton, KeywordClassifier> lexer;

/***********************************************************************************/

// un dfa est souvent:
//		- Un state de départ
//		- une variable d'une type T
//		- Un state de sortie

// Une subtibilité est que si on met plusieurs entrées dans le même lexer 
// ou la variable T n'est pas le même type partout, il faut soit lancer un erreur
// en effectuant un static_assert avant chaque application de regle ou bien SAUTER
// l'itération courante et passer au prochain si le type ne correspond pas. À VOIR.

// De plus, il faudrait être en mesure de supporter autant les fonctions que les variables
// elles mêmes. Il faudrait être en mesure d'injecter 'c' ou bien is_digit (une fonction 
// prenant la variable en question et qui retourne true/false) et que ça fonctionne.

/***********************************************************************************/

enum class LexingStateType {
    STATE_START,
    STATE_INVALID,
    STATE_ERROR,

    STATE_IDENTIFIER,
    STATE_DELIMITER,

    STATE_DELIM_COLON,
    STATE_DELIM_SEMI,
    STATE_DELIM_COMA,

    STATE_DELIM_R_PAREN,
    STATE_DELIM_L_PAREN,

    STATE_DELIM_R_CURLY,
    STATE_DELIM_L_CURLY,

    STATE_DELIM_R_SQUARE,
    STATE_DELIM_L_SQUARE,

    STATE_DELIM_R_ANGLE,
    STATE_DELIM_L_ANGLE,

    STATE_HASH,//deprec
    STATE_PREPROCESSOR,
    STATE_NEWLINE,


    STATE_OPERATOR,

    STATE_NUMBER,

    STATE_WHITESPACE,
};

/***********************************************************************************/

template<auto...>
struct dfa_transition;

template<auto Source, auto Predicate, auto Target>
struct dfa_transition<Source, Predicate, Target> {

    static constexpr auto source = Source;

    static constexpr auto predicate = Predicate;
    static constexpr auto target = Target;
};

template<auto Source, auto Predicate, auto Target>
struct ENABLED : dfa_transition<Source, Predicate, Target> {};



template<LexingStateType Source, char Predicate, LexingStateType Target>
struct dfa_entry_for_enum {
    static constexpr LexingStateType source = Source;

    static constexpr auto predicate = Predicate;
    static constexpr LexingStateType target = Target;
};

// probablement toujours préférer faire genre des fonctions 

//  struct IsNewline {
//      constexpr bool operator()(char c) const {
//          return c == '\n';
//      }
//  };

// tout dépendemment du design que j'opte

template<auto V>
struct InvalidEnumStateFallback {
    [[nodiscard]] static constexpr auto run(const auto, const auto) noexcept {
        return V;
    }
};

struct ExceptionFallback {
    [[noreturn]] static constexpr auto run(auto, auto) noexcept {
        throw std::runtime_error("no transition");
    }
};


template<typename Fallback, typename... Entries>
struct DeterministicFiniteAutomaton final {
protected:
    [[nodiscard]] static constexpr auto resolve_recursively(const auto& state, const auto& argument)
        noexcept(noexcept(Fallback::run(state, argument)))
    {
        return Fallback::run(state, argument);
    }

    template<typename Current, typename... Remaining>
    [[nodiscard]] static constexpr auto resolve_recursively(const auto& state, const auto& argument)
        noexcept(
            noexcept(state == Current::source) &&
            noexcept(argument == Current::predicate) &&
            noexcept(resolve_recursively<Remaining...>(state, argument))
        )
    {
        if (state == Current::source && argument == Current::predicate)
            [[unlikely]] return Current::target;

        return resolve_recursively<Remaining...>(state, argument);
    }

public:
    [[nodiscard]] static constexpr auto step(const auto& state, const auto& argument)
        noexcept(noexcept(resolve_recursively<Entries...>(state, argument)))
    {
        return resolve_recursively<Entries...>(state, argument);
    }
};

// n'est pas autant optimal que switch mais plus flexible
// il est O(n) n étant le nombre d'itérations avant la correspondance

// mais pour peu de entries, c'est a peu près la meme chose qu'un switch niveau perf.
  

/******************************************************************************/

struct LexingAutomaton final {
    
    constexpr LexingStateType step(const LexingStateType state, const char c) {
        switch (state) {
        case LexingStateType::STATE_START:
            /*if (std::isalpha(c)) return LexingStateType::STATE_IDENTIFIER;
            else if (std::isdigit(c)) return LexingStateType::STATE_NUMBER;
            else if (c == ':') return LexingStateType::STATE_DELIM_COLON;
            else if (c == ';') return LexingStateType::STATE_DELIM_SEMI;
            else if (c == ',') return LexingStateType::STATE_DELIM_COMA;
            else if (c == '(') return LexingStateType::STATE_DELIM_L_PAREN;
            else if (c == ')') return LexingStateType::STATE_DELIM_R_PAREN;
            else if (c == '{') return LexingStateType::STATE_DELIM_L_CURLY;
            else if (c == '}') return LexingStateType::STATE_DELIM_R_CURLY;
            else if (c == '[') return LexingStateType::STATE_DELIM_L_SQUARE;
            else if (c == ']') return LexingStateType::STATE_DELIM_R_SQUARE;
            else if (c == '<') return LexingStateType::STATE_DELIM_L_ANGLE;
            else if (c == '>') return LexingStateType::STATE_DELIM_R_ANGLE;*/



            if (c == '\n') return LexingStateType::STATE_NEWLINE;



            /*else if (isdelimiter(c)) return LexingStateType::STATE_DELIMITER;
            else if (isoperator(c)) return LexingStateType::STATE_OPERATOR;
            else if (iswhitespace(c)) return LexingStateType::STATE_WHITESPACE;

            else if (ispreprocessor(c)) return LexingStateType::STATE_PREPROCESSOR;

            else return LexingStateType::STATE_INVALID;



        case LexingStateType::STATE_PREPROCESSOR:
            if (std::isalpha(c)) return LexingStateType::STATE_PREPROCESSOR;

            //if (c != '\n') return LexingStateType::STATE_PREPROCESSOR;

            else return LexingStateType::STATE_INVALID;

            //case LexingStateType::STATE_NEWLINE:
            //    if (std::isalpha(c)) return LexingStateType::STATE_PREPROCESSOR;
            //    else return LexingStateType::STATE_INVALID;





        case LexingStateType::STATE_IDENTIFIER:
            if (std::isalpha(c) || std::isdigit(c)) return LexingStateType::STATE_IDENTIFIER;
            else return LexingStateType::STATE_INVALID;

        case LexingStateType::STATE_NUMBER:
            if (std::isdigit(c)) return LexingStateType::STATE_NUMBER;
            else return LexingStateType::STATE_INVALID;

        case LexingStateType::STATE_OPERATOR:
            if (isoperator(c)) return LexingStateType::STATE_OPERATOR;
            else return LexingStateType::STATE_INVALID;

        case LexingStateType::STATE_DELIMITER:
            return LexingStateType::STATE_INVALID;

        case LexingStateType::STATE_DELIM_COLON:
            if (c == ':') return LexingStateType::STATE_DELIM_COLON;
            else return LexingStateType::STATE_INVALID;

        case LexingStateType::STATE_WHITESPACE:
            if (iswhitespace(c)) return LexingStateType::STATE_WHITESPACE;
            else return LexingStateType::STATE_INVALID;*/
        }
        return LexingStateType::STATE_INVALID;
    }
};

/******************************************************************************/


// utiliser ENABLED, DISABLED, ETC...

export int main2() {
    using Automaton = DeterministicFiniteAutomaton <
        InvalidEnumStateFallback<LexingStateType::STATE_INVALID>,

        dfa_transition<LexingStateType::STATE_START, '\n', LexingStateType::STATE_NEWLINE>
    >;

    Automaton dfaA;

    auto res = dfaA.step(LexingStateType::STATE_START, '\n');

    std::cout << static_cast<int>(res) << "\n";


    /*************************************************************************/


    auto state = LexingStateType::STATE_START;
    auto arg = '\n';

    volatile auto sinkA = 0;


    auto startA = std::chrono::steady_clock::now();

    for (int i = 0; i < 1'000; ++i)
        sinkA ^= static_cast<int>(Automaton::step(state, arg));

    auto endA = std::chrono::steady_clock::now();

    auto durationA = std::chrono::duration_cast<std::chrono::nanoseconds>(endA - startA);

    std::cout << "[A] " << durationA << "\n";


    /*************************************************************************/

    LexingAutomaton dfaB;

    volatile auto sinkB = 0;

    auto startB = std::chrono::steady_clock::now();

    for (int i = 0; i < 1'000; ++i)
        sinkB ^= static_cast<int>(dfaB.step(state, arg));

    auto endB = std::chrono::steady_clock::now();

    auto durationB = std::chrono::duration_cast<std::chrono::nanoseconds>(endB - startB);

    std::cout << "[B] " << durationB << "\n";

    /*************************************************************************/


    return 0;
}
