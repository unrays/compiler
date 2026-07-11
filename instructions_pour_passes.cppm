#include <cstdint>
#include <array>
#include <sstream>
#include <bitset>
#include <type_traits>
#include <iostream>
#include <memory_resource>
#include <string_view>

export module instruction_things;

import sharded_string_interner_module;
import fixed_string_module;

using NodeType = int;
//using TokenType = int;

using MockModuleA = std::uint8_t;
using MockModuleB = std::uint16_t;
using MockModuleC = std::uint32_t;
using MockModuleD = std::uint64_t;

using MockModuleShared = std::size_t;

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_constructive_interference_size;
using std::hardware_destructive_interference_size;
#else
// 64 bytes on x86-64 | L1_CACHE_BYTES | L1_CACHE_SHIFT | __cacheline_aligned | ...
constexpr std::size_t hardware_constructive_interference_size = 64;
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif

/************************************************************************************************************/

// en gros on pars avec un modèle DoD.

// ce qu'on va faire c'est refactoriser l'ancien système de l'ESP afin qu'il se rapproche un peu
// plus des pratiques communes des compilateurs.

// voici le plan de match :
//	1. Il s'agirait de garder plusieurs registres DoD différents et accessibles par les passes.
//		l'option de mettre les registres dans le SharedContext me semble un peu bad mais il s'agirait
//		d'explorer un peu la chose afin de bien comprendre les enjeux.
//
//	2. Au lieu de faire une passe qui transforme les Tokens en un arbre intermédiaire composé de TokenNode, 
//		il s'agirait plutot de TOUT DE SUITE créer la structure de l'arbre avec les Nodes qui contiennent des IDs.
//		Ensuite, il s'agirait de stocker les Tokens (probablement faire un array avec taille fize afin que ça soit contigue)
//		dans un registre DoD. Ainsi, chaques IDs permettraient d'accéder aux NodeTokenData des noeuds. 
// 
//	3. Ensuite, la structure restant semblablement la même que la passe précédente, l'arbre pourrait être réutilisé sans
//		aucune modification (étant donné que les noeuds sont seulement composés de IDs). Il est à noter qu'il serait mieux
//		de stocker le type de noeud dans les node data eux-mêmes ou quelque chose du genre afin de ne pas bind les Id holders
//		aux NodeTypes. Enfin, au niveau de l'algorithme, étant donné que les IDs restent les mêmes, il ne s'agirait que de
//		parcourir le registre de TokenData en transformant chacunes des suites de Tokens en NodesData avec l'algorithme.
//		De cette façon, il serait très facile d'entrer en parallèle (possiblement utiliser SIMD) les nouveaux NodeData dans
//		un registre de NodeData aux mêmes IDs que ceux de l'autre registre (en spécifiant leur type possiblement). Ainsi,
//		peu importe la structure de l'arbre, nous aurons chacuns des noeuds transformés et de façon extrèmement rapide. 
// 
//		- peut-être un TypeRegistry mais c'est beaucoup moins performant que de le mettre directement.
// 
//	4. Enfin, il s'agirait de réfléchir à une façon de transformer l'arbre AST en un flatten tree. En gros, c'est le même
//		principe que les DFA avec les matrices compile-time, nous avons un nombre fixe de noeuds enfants qu'un noeud peut
//		avoir (ligne, notée i) et les noeuds eux-mêmes (colonnes, notée j). 
// 
//

// peut-être même mettre un nombre max de Tokens dans le TokenData

/************************************************************************************************************/

// suite:
//		E1. Pour l'algorithme de création de la structure de l'arbre avec les NodeTokenData, il faut changer adapter
//			l'approche afin de tirer un maximum de performances. Pour se faire, il faut remplacer le std::array
//			dans le NodeTokenData par deux champs; begin (ptr/ID) et end (ptr/ID).
//
//		E2. Pour ce qui est de l'algorithme de la création, en gros, il faut placer tous les Tokens (ou autre) dans
//			une plage contigue (monotonic?). Ensuite, l'algorithme va créer un premier NodeTokenData en lui assignant
//			un pointeur sur la position du curseur de la plage sur son champ 'begin' (le début dans ce cas) et nous
//			avancons jusqu'a ce qu'il arrive sur une coupure (rules policies). Quand on arrive sur la coupure,
//			on assigne le 'end' avec un pointeur sur le curseur actuel. Enfin, on continue ceci jusqu'à ce que nous
//			arrivions à la fin des Tokens.
// 
//		E3. Il faut choisir entre un ID ou un ptr, lequel prends le plus de mémoire, quels sont les limitations, toutes
//			ces choses seront à analyser. Je penses que le ID serait mieux seulement s'il fait moins de 8 bytes. Faut aussi
//			s'assurer du lifetime des données car ça semble se complexifier rapidement.

/************************************************************************************************************/

// pour les contexts, il n'y a rien de mal a partager des registres entre les passes.
// aussi, il faut créer un CompilationUnit qui contient toutes les dépendances d'un seul coup et référencer
// les dépendances dans les différents sous-contexts.

// le shared context est réservé au partage dans tout le compilateur au complet, pas juste ce unit en particulier.

/************************************************************************************************************/

struct SharedContext {
	// dans le shared, on peux mettre par exemple:
	//      - un systeme de debugging
	//      - peut-être un allocator "fourre-tout"
	//      - SURTOUT le StringInterner
	//      - la table de symbole


	ShardedStringInterner<10> stringInterner;
};



// peut etre faire une classe abstraite avec le SharedContext& et constructeur approprié

struct alignas(hardware_constructive_interference_size) StringSplitContext_lui_qui_faut_garder {
	explicit StringSplitContext_lui_qui_faut_garder(SharedContext& s)
		: shared(s) {
	} // temporaire attendant abstract class

public:
	SharedContext& shared;

};


struct alignas(hardware_constructive_interference_size) LexingContext_lui_qui_faut_garder {
	explicit LexingContext_lui_qui_faut_garder(SharedContext& s)
		: shared(s) {
	} // temporaire attendant abstract class

public:
	SharedContext& shared;

};

struct alignas(hardware_constructive_interference_size) ParsingContext_lui_qui_faut_garder {
	explicit ParsingContext_lui_qui_faut_garder(SharedContext& s)
		: shared(s) {
	}

public:
	SharedContext& shared;

};

struct alignas(hardware_constructive_interference_size) CodegenContext_lui_qui_faut_garder {
	explicit CodegenContext_lui_qui_faut_garder(SharedContext& s)
		: shared(s) {
	}

public:
	SharedContext& shared;

	// il ne faut jamais stocker l'ast ou un ARG venant de la passe
	// précédente dans les contextes des passes suivantes. pourquoi?

	// c'est simplement une question de ownership, le mémoire de l'ast
	// est allocated dans le arena allocator du contexte précédent.

	// nous devons consommer l'ast au lieu de le stocker, c'est le seul
	// compromis possible si je souhaite respecter le modèle mémoire actuel.
};

/************************************************************************************************************/

namespace PASS1_STRING_SPLITTING {


	struct StringSplittingContext {
		std::string_view input;
		std::vector<std::string>& accumulator;

		explicit StringSplittingContext(std::string_view input_,
			std::vector<std::string>& accumulator_)
			: input(input_)
			, accumulator(accumulator_) {
		}
	};


	/*****************************************************************/

	struct StringStreamSplitStrategy {
	public:
		void apply(StringSplittingContext& context) const {
			std::stringstream ss(std::string(context.input));
			ss.unsetf(std::ios::skipws);

			char c;
			std::string current;

			while (ss.get(c)) {
				if (c == '\n') {
					if (!current.empty()) {
						context.accumulator.push_back(current);
						current.clear();
					}
					context.accumulator.push_back("\n");
				}
				else if (std::isspace(static_cast<unsigned char>(c))) {
					if (!current.empty()) {
						context.accumulator.push_back(current);
						current.clear();
					}
				}
				else {
					current.push_back(c);
				}
			}

			if (!current.empty())
				context.accumulator.push_back(current);
		}
	};

	/*****************************************************************/


	template<typename Strategy>
	struct StringSplitter {
		[[nodiscard]] const auto split(const std::string_view& input) const
		{
			std::vector<std::string> accumulator; //FAUT FAIRE DES STRING_VIEWS
			auto context = StringSplittingContext{ input, accumulator };

			Strategy{}.apply(context);

			return context.accumulator;
		}
	};


	/*****************************************************************/
	/*****************************************************************/
	/*****************************************************************/

	/******/

	// noexcept((noexcept(CompositionPolicies::compose(composand)) && ...))

	template<char... Principles>
	struct CharMatchingPolicy {
		static constexpr [[nodiscard]] bool matches(char c)
			noexcept((noexcept(c == Principles) && ...))
		{
			return ((c == Principles) || ...);
		}
	};

	using WhitespaceMatchingPolicy = CharMatchingPolicy<'\x20'>;
	using NewLineMatchingPolicy = CharMatchingPolicy<'\x0A'>;


	template<typename... FilterPolicies>
	struct StreamSegmentationStrategy { // peut-être fusionner avec celui d'en bas maintenant que la stratégie est plus générique
	public:
		static [[nodiscard]] std::vector<std::string_view> execute(std::string_view input) {
			std::vector<std::string_view> buffer;
			buffer.reserve(1 << 8); // 256

			const char* start = input.data();
			const char* end = start + input.size();
			const char* current_seg_start = start;

			for (const char* p = start; p != end; ++p) {
				if ((FilterPolicies::matches(*p) || ...)) {
					buffer.emplace_back(current_seg_start, static_cast<std::ptrdiff_t>(p - current_seg_start));
					current_seg_start = p;
				}
			}

			buffer.emplace_back(current_seg_start, static_cast<std::ptrdiff_t>(end - current_seg_start));
			return buffer;
		}
	};



	template<typename SegmentationStrategy>
	struct StringSegmenter final {
	public:
		[[nodiscard]] std::vector<std::string_view> segment(std::string_view input)
			noexcept(noexcept(strategy.execute(input)))
		{
			return strategy.execute(input);
		}

	private:
		[[no_unique_address]] SegmentationStrategy strategy;

	};


	// en gros, je peux garder les \n et espaces pour la phase de segmentation et créer des tokens
	// avec les informations nécessaires (genre ligne et tout) au moment du lexical analyser qui au lieu
	// de transformer \n en token, va simplement executer une règle spécifique poru changer de ligne le compteur

	// pour calculer les lignes et colonnes

	// token { ligne; colonne; }

	// FINALEMENT CE MODULE EST INUTILE CAR NOUS N'AVONS PAS BESOIN DE DÉCOUPER LE CONTENU POUR QUE LE LEXER PUISSE FONCTIONNER,
	// C'EST PROBABLEMENT UN ARTÉFACT DE L'ANCIENNE ARCHITECTURE.
}


namespace PASS2_CONTENT_LEXING {


/********************************************************************************/


#if 0
	struct Stateful;
	struct Stateless;



	// StreamReader - stateful
	// PositionTracker - stateful
	// Lexer



	template<typename Policy>
	struct PositionTrackerr {

		// genre en gros, on demande a ce que le contexte donné ait les mêmes données que le contexte stateful du module
		// ca permet de faire un architecture autant stateless que stateful


		/*    using MemberType = std::conditional_t<KeepMember, int, empty_tag>;

			  // [[no_unique_address]] ensures empty_tag occupies 0 bytes
			  [[no_unique_address]] MemberType data;
		*/
		
		struct StatelessData {};

		struct StatefulData {
			int line;
			int column;
		};

		using DataType = std::conditional_t<std::is_same_v<Policy, Stateful>, StatefulData, StatelessData>;



		/*void foo(DataType& p = data) {

			if constexpr (std::is_same_v<DataType, StatelessData>) {

			}

		}*/



		// au pire faire version statique et non statique


		// delete le overload si par exemple le Policy est Stateful
		static void foo_static(StatefulData p) { //passer par ref
			std::cout << "[STATIC OVERLOAD] p.column -> " << p.column << "\n";
		}

		void foo() {
			std::cout << "[NON-STATIC OVERLOAD] p.column -> " << data_test.column << "\n";
		}

		void foo2(this PositionTrackerr<Stateful> self) {

		}

		void foo2(this PositionTrackerr<Stateless> self, StatefulData p) {

		}


		/*void foo(StatefulData& p) {
			std::cout << "p.column -> " << p.column << "\n";
		}*/


		StatefulData data_test;

		[[no_unique_address]] DataType data;

	};
#endif


/********************************************************************************/


	enum class LexingStateType { // PEUT-ÊTRE À CHANGER, SET UTILISÉ DANS L'ESP ET TRÈS ORIENTÉ C++
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


/********************************************************************************/


	// TOTALEMENT NON TERMINÉ, IL FAUT ABSOLUMENT LE PAUFINER ET LE RENDRE PLUS STABLE, FONCTIONNE TEMPORAIREMENT
	template<std::size_t X, std::size_t Y, typename... Entries> // mettre aussi les types admis
	struct CompileTimeDfa {
	public:

		//table[state * N + input]


		// genre is_catable_to_int, on recoit des types, pas des int directement


	public:
		[[nodiscard]] static constexpr int step_raw(int state, int input) {
			return table[state * X + input];
		}


		// must be castable to integer, le return type mettre le type d'entrée state



		// OK JE DEVRAIS FAIRE DEUX MODULES, EN GROS YA LE COMPILE TIME DFA ET 
		// YA UN WRAPPER PAR DESSUS QUI EFFECTUE TOUT LE TRUC CHIANT AVEC LES MODULES

		/*[[nodiscard]] static constexpr int step(int state, int input) noexcept {
			return table[static_cast<int>(state) * X + static_cast<int>(input)];
		}*/

		template<typename Tp, typename Up>
		[[nodiscard]] static constexpr Tp step(Tp state, Up input) noexcept(false) {
			return static_cast<Tp>(
				table[static_cast<int>(state) * X + static_cast<int>(input)]
			);
		}

		template<typename Tp, typename Up>
			requires std::convertible_to<Tp, int>&&
		std::convertible_to<Up, int>&&
			std::convertible_to<int, Tp>
			[[nodiscard]] static constexpr Tp step_with_concept(Tp state, Up input) noexcept {
			return static_cast<Tp>(
				table[static_cast<int>(state) * X + static_cast<int>(input)]
			);
		}

	public:

		static constexpr auto table = [] {
			std::array<int, X* Y> t{};

			if constexpr (X * Y > 10) { // teporaire mock


				((
					t[static_cast<int>(Entries::source) * X + static_cast<int>(Entries::predicate)]
					= static_cast<int>(Entries::target)
					), ...);

			}

			//if constexpr (X * Y > 10) {
			//    //t[0][10] = 42;




			//    t[0 * X + 10] = 42;
			//}

			return t;
			}();



		/*static constexpr int table2[X * Y] = {
			-1, -1, -1
		}*/


		//static constexpr int table2[N] = [] {
		//    //std::array<int, N> t{};

		//    int t[N]{};

		//    if constexpr (N > 10) {
		//        //t[0][10] = 42;




		//        t[0 * N + 10] = 42;
		//    }

		//    return t;
		//}();;



	};


/********************************************************************************/


	template<fixed_string... Principles>
	struct KeywordMatchingPolicy {
		static constexpr [[nodiscard]] bool matches(std::string_view sv)
			noexcept((noexcept(sv == Principles) && ...))
		{
			return ((sv == Principles) || ...);
		}
	};


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


	enum struct TokenType {
		Identifier,

		Keyword,

		Kwrd_Type,
		Kwrd_Qualifier,
		Kwrd_Specifier,
		Kwrd_Modifier,
		Kwrd_Alignment,
		Kwrd_Control,
		Kwrd_Access,

		Delimiter,

		Delim_Colon,
		Delim_Semicolon,
		Delim_Coma,

		Delim_RParen,
		Delim_LParen,

		Delim_LCurly,
		Delim_RCurly,

		Delim_RSquare,
		Delim_LSquare,

		Delim_RAngle,
		Delim_LAngle,

		Preprocessor,
		Operator,
		Number,
		Whitespace,
		Newline,
		Invalid,
		Unknown
	};


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


	struct SourceLocation {
		std::size_t line;   // 8 bytes, std::uint16_t???
		std::size_t column; // 8 bytes
	}; 

	struct Token {
		//Token(TokenType k, const char)



		TokenType kind;


		//const char* lexeme; // string interné

		std::string_view lexeme; // TEMPORAIRE

		SourceLocation location;
	};


/********************************************************************************/


	struct CharReader { // changer le nom
	public:
		/*CharReader(std::string_view pcontent)
			: content(pcontent) {}*/


		CharReader(std::string_view source) // peut etre prendre directement begin et end
			: begin_(source.data())
			, end_(begin_ + source.size())
			, cursor_(begin_)
		{}


		CharReader(const char* begin, const char* end) // je pense que c'est lui qu'il faut garder
			: begin_(begin)
			, end_(end)
			, cursor_(begin)
		{
		}

	public:
		[[nodiscard]] const char* advance() {
			if (cursor_ == end_ || *cursor_ == '\0') [[unlikely]] {
				return cursor_;
			}

			return ++cursor_;
		}

		// fonction peek pour regarder le char actuel



	private:
		// advance

		//std::string_view content;


		////// return \0 si on est a la fin

		// PEUT-ÊTRE ALLOCATE LES CHAR OU QUELQUE CHOSE DU GENRE, À VOIR!

		const char* begin_;
		const char* end_;
		const char* cursor_;

	};


/********************************************************************************/


	struct PositionTracker {
	public:
		SourceLocation& update(char c) { // attention à la copie, sagirait d'investiguer le lifetime, copier reste extrememet cheap.
			
			if (c == '\n') {
				++current.line;
				current.column = 0;
			}
			// peut etre gerer les tab genre +4 ou +8
			else {
				++current.column;
			}

			return current;
		}

		SourceLocation& current_location() {
			return current;
		}




	private:
		SourceLocation current{}; // current_???

	};


/********************************************************************************/

	// DEVRAIS DEVENIR GENRE UNE CONFIGURATION PAR POLICY OU RULES, QUELQUE CHOSE DU GENRE.

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


/********************************************************************************/


	// LexicalAnalyzer<LexingAutomaton, KeywordClassifier> lexer;

	// template reader & tracker + probablement le automaton

	template<
		//typename Reader, typename Tracker,
		typename LexingAutomaton, typename Recategorizer
	>
	struct Lexer {


		// peut etre mettre soit les automaton et classifier
		// ou mettre le tracker et le reader, à voir...

		template <typename Reader, typename Tracker>
		[[nodiscard]] std::vector<Token> tokenize(std::string_view source) {
			Reader reader{ source.data(), source.data() + source.size() };
			Tracker tracker{};


			std::vector<Token> tokens;
			tokens.reserve(1 << 10); // 1024


			LexingStateType current_state = LexingStateType::STATE_START;

			const char* current_ptr = source.data();
			const char* current_seg_start = current_ptr;

			SourceLocation current_location;



			while (*current_ptr != '\0') { // peut-être une fonction genre is_end

				current_location = tracker.update(*current_ptr);
				LexingStateType next_state = automaton.step(current_state, *current_ptr); // peut-être assignation au lieu d'instantiation


				if (next_state == LexingStateType::STATE_INVALID) [[unlikely]] {
					tokens.emplace_back(
						state_to_token(current_state),
						std::string_view(current_seg_start, static_cast<std::ptrdiff_t>(current_ptr - current_seg_start)), // en attendant le interner
						current_location // par copie
					);

					current_seg_start = current_ptr; // pas 100% certain, à tester...
					current_state = LexingStateType::STATE_START;
				}
				else {
					current_ptr = reader.advance();
				}

			}


			return tokens; // pas copie... à voir pour allocation.

			// PEUT-ÊTRE FAIRE UN AUTER MODULE QUI TRAITE LES TOKENS A LA FIN AVEC LE CLASSIFIER. 
			// ÇA RENDRAIT UN PEU MOINS LOURD LA LOGIQUE DE CETTE CLASSE.









			// FAUT AUSSI INTERN LES STRINGS avant de les mettre dans les tokens, genre quand on coupe le char


			// on demande le prochain char au StreamReader avec .get_next() ou quelque chose du genre.

			// on passe le char lu au PositionTracker qui retourne la prochaine position en condition du charactère.

			// on exécute l'algorithme de l'automaton pour obtenir le prochain state.



			//PositionTrackerr<Stateful> stateful_tracker;
			//PositionTrackerr<Stateless> stateless_tracker;


		}


		[[no_unique_address]] LexingAutomaton automaton;
		[[no_unique_address]] Recategorizer recategorizer; // module séparé peut-être
	};


/********************************************************************************/


}










/************************************************************************************************************/

struct CompilationUnit {
	MockModuleShared shared;

	MockModuleA a;
	MockModuleB b;
	MockModuleC c;
	MockModuleD d;

	// contient tous les modules dont le shared par référence
};

/************************************************************************************************************/

struct ParserContext { //mock
public:
	explicit constexpr ParserContext(MockModuleA& pa, MockModuleC& pc)
		: a(pa), c(pc) {}

	MockModuleA& a;
	MockModuleC& c;
};

struct ParserContextCompositionPolicy final { // y'aurais moyen de rendre ca générique, à voir!
private:
	using Src = CompilationUnit;
	using Out = ParserContext;

	static constexpr bool is_create_nothrow = noexcept(Out{
		std::declval<Src&>().a,
		std::declval<Src&>().c
	});

public:
	static constexpr [[nodiscard]] Out compose(Src& unit) noexcept(is_create_nothrow) {
		return Out{ unit.a, unit.c };
	}

public:
	struct contract final {
		using RequiredSource = Src;
		using ExpectedOutput = Out;
	};
};

/************************************************************************************************************/

template<typename... CompositionPolicies>
struct ContextComposer final {
public:
	//explicit constexpr ContextComposer(CompilationUnit& unit) {} // en faite une fonction est probablement mieux


	// peut etre mettre compilationUnit en template et mettre requirement dans le policy


	template<typename Tp>
	static constexpr [[nodiscard]] decltype(auto) compose(Tp&& composand)
		noexcept((noexcept(CompositionPolicies::compose(composand)) && ...))
	{
		([&]() {
			static_assert(
				std::is_same_v<std::decay_t<Tp>, typename CompositionPolicies::contract::RequiredSource>,
				"Invalid input type: The decayed type of the argument must strictly match"
				"'RequiredSource' across all composition policies."
			);
		}(), ...);

		return std::tuple<typename CompositionPolicies::contract::ExpectedOutput...>{
			((CompositionPolicies::compose(std::forward<Tp>(composand))), ...)
		};
	}
};

/************************************************************************************************************/

// les sous-contexts ne sont que des vues (stockage par référence) des modules du CompilationUnit

template<typename... Models>
struct ContextProvider {
public:
	explicit constexpr ContextProvider(std::tuple<Models...>& ensemble)
		: contexts(ensemble) {}

public:
	template<typename T>
	constexpr T& get() noexcept {
		return std::get<T>(contexts);
	}

private:
	std::tuple<Models...> contexts;

};

/************************************************************************************************************/

struct Node { // ou plutot NodeHandle
	std::uint32_t id;
};

struct Token_old {
	//TokenType type;

	std::uint32_t lexeme; // id dans le string interner. utiliser la technique de UE5 discuté précédemment.

	//std::string lexeme;
};

struct NodeTokenData { // nom temporaire
	/*std::array<Token, 25> data; // 25 tokens maximum*/

	std::uint32_t begin; // 4 bytes
	std::uint32_t end;
};

struct NodeData {
	NodeType kind;

	// trouver une façon d'y foutre les données
};

struct FlattenedAST {
	static constexpr std::size_t i = 10;
	static constexpr std::size_t j = 10;

	std::bitset<i * j> flattened;
};


export void main_instruction_for_passes() {

	using ContextComposer = ContextComposer<ParserContextCompositionPolicy>;

	ContextComposer composer;
	CompilationUnit unit{ 1, 2, 3, 4 };

	auto ensemble = composer.compose(unit);
	ContextProvider<ParserContext> provider{ ensemble };

	std::cout << typeid(decltype(ensemble)).name() << "\n";
	std::cout << typeid(decltype(provider.get<ParserContext>())).name() << "\n";

	/*******************************************************************************/


	std::array<std::byte, 512> my_buffer;

	std::pmr::monotonic_buffer_resource arena(
		my_buffer.data(),
		my_buffer.size(),
		std::pmr::null_memory_resource()
	);

	std::pmr::monotonic_buffer_resource arena2(1024 * 1024, std::pmr::null_memory_resource());

	/*******************************************************************************/

	// il doit inter chacunes des strings qu'il découpe et il doit retourner un vecteur ou une plage de ptr/ids

	// ou peut-être qu'il devrait retourner les strings mais que un module pourrait repasser dessus pour transformer
	// en vector de stirng internés

	// ON MET LE STRING INTERNING DANS LE LEXER. LE LEXER RECOIT UN std::vector<std::string_view> et au fur et 
	// à mesure, il transforme les strv en interned.

	{
		using namespace PASS1_STRING_SPLITTING;

		std::string content = "hello world, j'aime le c++\n retour a la ligne";

		StringSplitter<StringStreamSplitStrategy> splitter;

		auto split1 = splitter.split(content);

		std::cout << "old system: ";
		for (auto const& s : split1) {
			std::cout << '[' << s << ']';
		}
		std::cout << "\n";

		/***************** NEW SEGMENTER ********************/

		using SegmentationStrategy = StreamSegmentationStrategy<
			WhitespaceMatchingPolicy,
			NewLineMatchingPolicy
		>;

		StringSegmenter<SegmentationStrategy> segmenter;

		auto split2 = segmenter.segment(content);

		std::cout << "new system: ";
		for (auto const& s : split2) {
			/*if (s == "") std::cout << "[BACKSLASH]";
			else std::cout << '[' << s << ']';*/

			std::cout << '[' << s << ']';
		}



		//StringSegmenter<>

	}


	{
		using namespace PASS2_CONTENT_LEXING;


		/*PositionTrackerr<Stateful> tracker{};

		tracker.foo();

		PositionTrackerr<Stateful>::foo_static({});

		tracker.foo2();

		PositionTrackerr<Stateless> stateless_tracker{};

		stateless_tracker.foo2({});*/


		/*
			template<
				typename Reader, typename Tracker,
				typename LexingAutomaton, typename Recategorizer
			>
		*/


		constexpr CompileTimeDfa<15, 15,

			dfa_transition<LexingStateType::STATE_START, '\n', LexingStateType::STATE_NEWLINE>
		> dfaD;

		using LexingAutomaton = CompileTimeDfa<15, 15,

			dfa_transition<LexingStateType::STATE_START, '\n', LexingStateType::STATE_NEWLINE>
		>;

		using TokenKwrdClassifier = TokenKeywordClassifier2<
			TokenClassifierContext<AccessKeywordMatchingPolicy, TokenType::Kwrd_Access>,
			TokenClassifierContext<AlignmentKeywordMatchingPolicy, TokenType::Kwrd_Alignment>
		>;

		using LexicalAnalyzer = Lexer<LexingAutomaton, TokenKwrdClassifier>;


		std::string content = "hello world, j'aime le c++\n retour a la ligne";

		LexicalAnalyzer lexer;

		std::vector<Token> tokens = lexer.tokenize<CharReader, PositionTracker>(content);

		std::cout << "Tokens: \n";
		for (auto const& t : tokens) {
			std::cout << '[' << t.lexeme << ']';
		}


	}

}
