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





template <typename T>
constexpr std::string_view type_name() {
#if defined(_MSC_VER)
	std::string_view signature = __FUNCSIG__;
	size_t prefix_pos = signature.find("type_name<");
	if (prefix_pos == std::string_view::npos) return signature;
	prefix_pos += 10;
	size_t suffix_pos = signature.rfind(">(void)");
	return signature.substr(prefix_pos, suffix_pos - prefix_pos);
#elif defined(__clang__) || defined(__GNUC__)
	std::string_view signature = __PRETTY_FUNCTION__;
	size_t prefix_pos = signature.find("T = ");
	if (prefix_pos == std::string_view::npos) return signature;
	prefix_pos += 4;
	size_t suffix_pos = signature.find_last_of(']');
	return signature.substr(prefix_pos, suffix_pos - prefix_pos);
#else
	return "ERROR";
#endif
}



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


	enum struct LexState : int {
		Invalid = -1,
		Start = 0,

		Identifier = 10,
		Operator = 11,
		Number = 12,

		DelimiterOpaque = 20,
		DelimiterColon = 21,
		DelimiterSemi = 22,
		DelimiterComma = 23,

		DelimiterRParen = 30,
		DelimiterLParen = 31,
		DelimiterLCurly = 32,
		DelimiterRCurly = 33,
		DelimiterRSquare = 34,
		DelimiterLSquare = 35,
		DelimiterRAngle = 36,
		DelimiterLAngle = 37,

		Preprocessor = 40,
		Newline = 41,
		Whitespace = 42
	};












/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/





#if 0
	template<typename Container>
	struct make_tuple_from_container;

	template<template<auto, typename...> typename Container, bool Condition, typename... Ts>
	struct make_tuple_from_container<Container<Condition, Ts...>> {
		using type = std::tuple<Ts...>;
	};

	template<typename Container>
	using make_tuple_from_container_t = typename make_tuple_from_container<Container>::type;



	// le static assert avec ErrMsg est uniquement C++26 alors il faudrait faire genre un SMART_ASSERT
	// ou quelque chose du genre avec #if c__plusplus

	template<
		/*ConfigurationEntryConcept*/ typename EntryT,
		std::size_t               ReqSize,
		fixed_string			  ErrMsg = "This is the default error message!"
	>
	struct ConfigurationConstraint { // changer le nom
	protected:
		static_assert(EntryT::size == ReqSize, "Constraint size mismatch!");
		using tuple_t = make_tuple_from_container_t<EntryT>;

	public:
		template<std::size_t N>
		using get_element_at_t = std::tuple_element_t<N, tuple_t>;
	}; // PEUT ETRE AUSSI FAIRE SYSTÈME QUI VÉRIFIE LES TYPES GENRE FAUT QUE 1 SOIT UN AUTOMATA

	template</*ConfigurationEntryConcept*/ typename EntryT>
	struct SystemConfigurationMask final : ConfigurationConstraint<EntryT, 2, "Custom mask error message."> {
	public:
		using rule_t = typename SystemConfigurationMask::template get_element_at_t<0>;
		using behavior_t = typename SystemConfigurationMask::template get_element_at_t<1>;
	};



	//template<template<typename> typename Mask, typename ConfigTuple>
	//struct apply_mask_on_container;

	//template< template<typename> typename Mask, template<typename> typename Container, ConfigurationEntryConcept... Ts>
	//struct apply_mask_on_container<Mask, Container<Ts...>> {
	//public:
	//	using as_tuple = std::tuple<Mask<Ts>...>;


	//};



	template<
		typename ConfigSystem,
		template<typename> typename Mask,
		template<typename...> typename TargetContainer
	>
	struct apply_mask_on_configuration_system final {
	public:
		using type = decltype([]<typename... Ts>(std::tuple<Ts...>) {
			return TargetContainer<Mask<Ts>...>{};
		}(std::declval<typename ConfigSystem::as_tuple>()));
	};

	template<
		typename ConfigSystem,
		template<typename> typename Mask,
		template<typename...> typename TargetContainer = std::tuple
	>
	using apply_mask_on_configuration_system_t =
		typename apply_mask_on_configuration_system<ConfigSystem, Mask, TargetContainer>::type;






	//genre utiliser SystemConfiguration PAR DESSUS le système SystemConfigurationMask

	// PEUT-ÊTRE FAIRE UNE BASE QUI EFFECTUE LE TRAVAIL EN RAPPORT AVEC LE CONFIG SYSTEM

	template<
		SystemConfigurationConcept SystemConfig,
		template<typename> typename MaskModel = SystemConfigurationMask
	>
	struct ConfigurableSystem {
	protected:
		using masked_configuration_t = apply_mask_on_configuration_system_t<SystemConfig, MaskModel>;

	public:
		void print_ruleset() {
			std::cout << "Content of ruleset from ConfigurableSystem<...>: \n";

			std::apply([](auto... m) {
				((
					//std::cout << "  entry: " << typeid(decltype(m)).name() << '\n',

					std::cout << "   filtered entry: \n",

					std::cout << "  \trule_t: " << typeid(typename decltype(m)::rule_t).name() << "\n",
					std::cout << "  \tbehavior_t: " << typeid(typename decltype(m)::behavior_t).name() << "\n\n"
					), ...);
				}, ruleset_);
		}



	private:
		masked_configuration_t ruleset_; // peut-être opter plus tard pour std::type_identity mais pour l'instant c'est ok

	};

#endif






/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/


			// en gros un seul entryconfiguration qui prends variadic, ce système contient sizeof... et un tuple pour tupleelem
		// pour chaques systèmes, on définit un masque qui, par exemple, définit que using SyntaxicRule = tuple_elmt(1), etc...
		// et on ignore le reste des arguments, on met soit un warning ou un static_assert directement. 



	template <auto Value>
	struct nttp_to_type {
		static constexpr auto value = Value;

		using value_type = decltype(Value);

		constexpr operator value_type() const noexcept { return value; }
		constexpr value_type operator()() const noexcept { return value; }
	};



	template<typename T>
	concept ConfigurationEntryConcept = requires {
		{ T::condition } -> std::same_as<bool>;
		{ T::size } -> std::same_as<std::size_t>;
	};

	template<bool Condition, typename... Ts>
	struct GenericConfigurationEntry {
		static constexpr bool condition = Condition; // ajouté par la suite, il est probablement a la bonne place

		static constexpr std::size_t size = sizeof...(Ts);
	};








	template<typename Container>
	struct make_tuple_from_config_entry;

	template<bool Condition, typename... Ts>
	struct make_tuple_from_config_entry<GenericConfigurationEntry<Condition, Ts...>> {
		using type = std::tuple<nttp_to_type<Condition>, Ts...>;
	};

	template<typename Container>
	using make_tuple_from_config_entry_t = typename make_tuple_from_config_entry<Container>::type;







	template<typename Configuration>
	struct ModularSystem {

	};


	/********************************************************************************/

	// NOUVEAU SYSTÈME DE CONFIG




	template<typename... Ts>
	using ENABLED = GenericConfigurationEntry<true, Ts...>;

	template<typename... Ts>
	using DISABLED = GenericConfigurationEntry<false, Ts...>;

	template<auto Condition, typename... Ts>
	using CONDITIONAL = GenericConfigurationEntry<Condition, Ts...>;



	template<typename... Ts>
	struct ConfigurationFilterer;

	template<>
	struct ConfigurationFilterer<> {
		using as_tuple = std::tuple<>;
	};

	template<typename First, typename... Rest> // must be ConfigurationRule
	struct ConfigurationFilterer<First, Rest...> {
		using Tail = typename ConfigurationFilterer<Rest...>::as_tuple;

		using as_tuple = std::conditional_t <
			First::condition,
			decltype(std::tuple_cat(std::tuple<First>{}, Tail{})),
			Tail
		> ;
	};

	/***/



	template<typename T>
	struct enabled_disabled_filter {
		static constexpr bool value = T::condition;
	};

	template<typename T, template<typename> typename Predicate>
	using filter_element_t = std::conditional_t<Predicate<T>::value, std::tuple<T>, std::tuple<>>;

	template<typename T, template<typename> typename Predicate>
	struct FilterHelper;

	template<typename... Args, template<typename> typename Predicate>
	struct FilterHelper<std::tuple<Args...>, Predicate> {
		using type = decltype(std::tuple_cat(std::declval<filter_element_t<Args, Predicate>>()...));
	};





	template<typename T>
	struct transform_configuration_entry_in_tuple;

	template<template<auto, typename...> typename Entry, bool Condition, typename... Ts>
	struct transform_configuration_entry_in_tuple<Entry<Condition, Ts...>> {
		using type = std::tuple<Ts...>;
	};

	template<typename T, template<typename> typename Predicate>
	using transform_element_t = typename Predicate<T>::type;

	template<typename T, template<typename> typename Predicate>
	struct TransformationHelper;

	template<typename... Args, template<typename> typename Predicate>
	struct TransformationHelper<std::tuple<Args...>, Predicate> {
		using type = std::tuple<transform_element_t<Args, Predicate>...>;
	};





	template<
		//template<typename> typename Filter, // peut etre le mettre par defaut
		template<typename> typename Schema,
		template<typename> typename Model,
		typename... Entries // mettre concept pour entries
	>
	struct new_ConfigurationSystem {
	protected:
		using base_tuple = std::tuple<Entries...>;

		using filtered_tuple = typename FilterHelper<base_tuple, enabled_disabled_filter>::type;
		using transformed_tuple = typename TransformationHelper<filtered_tuple, transform_configuration_entry_in_tuple>::type;

		/*static constexpr bool schema_validation =
			[]<typename... Ts>(std::tuple<Ts...>) {
				return (Schema<Ts>::valid && ...);
			}(std::declval<transformed_tuple>());*/

		static constexpr bool schema_validation =
			[]<typename... Ts>(std::tuple<Ts...>) {
				return (Schema<Ts>::valid && ...);
			}(transformed_tuple{});

		static_assert(
			schema_validation,
			"[Config Error] This transformed entry type violates the strict structural requirements defined by your Schema."
		);

		using final_type =
			decltype([]<typename... Ts>(std::tuple<Ts...>) {
				return std::tuple<Model<Ts>...>{};
			}(std::declval<transformed_tuple>()));

	public:
		using type = final_type;

	};

	//template<
	//	template<typename> typename Schema,
	//	template<typename> typename Model,
	//	typename... Entries
	//> using new_ConfigurationSystem_t



	// recoit pleins de config, on crée un tuple contenant toutes les config en filtrant (ENABLED, DISABLED)
	// ensuite, pour chaques config du tuple, on crée un tuple avec les arguments 
	// pour chaques entry que l'on transforme, on l'es met dans le schema et model et on les push dans un nouveau tuple.

	// on recoit UN SEUL ENTRY sous forme de tuple (ses arguments)

	// utiliser VALUE_T pour les values (NTTP) dans les tuples

	template<typename entry_tuple>
	struct ConfigurationSchema {
		static constexpr bool valid =
			requires {
			requires std::tuple_size_v<entry_tuple> == 2;
		//requires std::same_as<std::tuple_element_t<0, entry_tuple>, MockModuleA>;
		};

		// JE SAIS PAS SI JE MET LE STATIC ASSERT ICI, SI CA FAIT EN SORTE QUE LA SYNTAXE DEVIENT ROUGE QUAND
		// JE MET QUELQUE CHOSE DE NON AUTORISÉ, à tester
	};


	template<typename entry_tuple>
	struct ConfigurationModel final {
		using rule_t = std::tuple_element_t<0, entry_tuple>;
		using behavior_t = std::tuple_element_t<1, entry_tuple>;
	};


	template<typename... Entries>
	using LambdaSystemConfiguration = new_ConfigurationSystem<
		ConfigurationSchema,
		ConfigurationModel,
		Entries...
	>;

	// ensuite, il ne manque plus qu'a faire plusieurs systemConfig par système genre
	// par sections




/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
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


	template<std::size_t R, std::size_t C>
	struct StaticMatrix final {
	public:
		template<typename... Args>
			requires (sizeof...(Args) == (R * C))
				  && (std::convertible_to<Args, int> && ...)
		consteval StaticMatrix(Args... entries)
			: m_{ entries... } {}

		consteval StaticMatrix(std::array<int, R * C> arr)
			: m_{ arr } {}

	public:
		[[nodiscard]] constexpr const auto& at(std::size_t i, std::size_t j) const noexcept {
			#if 0
			if (i >= R || j >= C) [[unlikely]]
				throw std::out_of_range("Matrix out of bounds.");
			#endif

			if (i == -1 || i < 0) [[unlikely]] { // pas propre mais fonctionnel
				static int invalid_state_fallback = -1;
				return invalid_state_fallback;
			}

			return m_[i * C + j];
		}

		[[nodiscard]] constexpr auto& at(std::size_t i, std::size_t j) noexcept {
			#if 0
			if (i >= R || j >= C) [[unlikely]]
				throw std::out_of_range("Matrix out of bounds.");
			#endif

			if (i == -1 || i < 0) [[unlikely]] { // pas propre mais fonctionnel
				static int invalid_state_fallback = -1;
				return invalid_state_fallback;
			}

			return m_[i * C + j];
		}

		// AJOUTER D'AUTRES CHOSES DANS LE FUTUT MAIS POUR L'INSTANT, C'EST FONCTIONNEL
		// operators [] ou () sans aucune vérif de bounds

	private:
		std::array<int, R * C> m_;
	};


/********************************************************************************/


	template<std::size_t RS, std::size_t RC>
	struct FlatMatrixDFA  final {
	protected:
		using integer_type = int;

		static constexpr integer_type default_state_start = 0;
		static constexpr integer_type default_invalid_state = -1;

		static constexpr std::array<integer_type, RS * RC> default_matrix_table = []() {
			std::array<integer_type, RS * RC> temp;
			temp.fill(-1);
			return temp;
		}();
		
	public:
		explicit consteval FlatMatrixDFA(
			std::array<integer_type, RS * RC> table = default_matrix_table,
			integer_type start_state   = default_state_start,								 
			integer_type invalid_state = default_invalid_state
		)	
			: matrix_{ table }
			, start_state_(start_state)
			, invalid_state_(invalid_state)
		{}

	public:
		[[nodiscard]] constexpr bool step(int predicate) {
			if (predicate < 0 || predicate >= RC) [[unlikely]] {
				return false;
			}

			previous_state_ = current_state_;
			current_state_ = matrix_.at(current_state_, predicate);

			return current_state_ != default_invalid_state;
		}

		[[nodiscard]] constexpr int get_current() const noexcept {
			return current_state_;
		}

		[[nodiscard]] constexpr int get_previous() const noexcept {
			return previous_state_;
		}

	public:




	public:
		void reset() noexcept {
			current_state_ = start_state_;
			previous_state_ = start_state_;
		}


	private:
		StaticMatrix<RS, RC> matrix_;

		integer_type start_state_;
		integer_type invalid_state_;

		integer_type current_state_  = start_state_;
		integer_type previous_state_ = start_state_;

	};



/***********************************************************************************/








	template<typename entry_tuple>
	struct StaticDFAConfigurationSchema {
		using first_t = std::decay_t<std::tuple_element_t<0, entry_tuple>>;
		using second_t = std::decay_t<std::tuple_element_t<1, entry_tuple>>;
		using third_t = std::decay_t<std::tuple_element_t<2, entry_tuple>>;

		static constexpr bool valid =
			requires {
				requires std::tuple_size_v<entry_tuple> == 3;

				requires !std::same_as<decltype(first_t::value), decltype(second_t::value)>;
				requires std::same_as<decltype(first_t::value), decltype(third_t::value)>;
		};
	};

	template<typename entry_tuple>
	struct StaticDFAConfigurationModel final {
		static constexpr auto source = std::tuple_element_t<0, entry_tuple>::value;

		static constexpr auto predicate = std::tuple_element_t<1, entry_tuple>::value;
		static constexpr auto target = std::tuple_element_t<2, entry_tuple>::value;
	};




	template<typename... Entries>
	using StaticDfaTransitions = new_ConfigurationSystem<
		StaticDFAConfigurationSchema,
		StaticDFAConfigurationModel,
		Entries...
	>;

	template <typename T>
	constexpr bool is_static_dfa_config_v = false;

	template <typename... Entries>
	constexpr bool is_static_dfa_config_v<StaticDfaTransitions<Entries...>> = true;

	template <typename T>
	concept is_static_dfa_configuration = is_static_dfa_config_v<T>;


	// dfa_transition<LexState::Start, '\n', LexState::Newline>





	template<is_static_dfa_configuration Configuration>
	struct StaticDFA  final { // genre configurable ou modularDfa
	protected:
		using integer_type = int;
		using configuration_tuple_t = typename Configuration::type;

	protected:
		using first_entry_type = std::tuple_element_t<0, configuration_tuple_t>;

		using row_type = std::decay_t<decltype(first_entry_type::source)>; // genre remove ou decay
		using column_type = std::decay_t<decltype(first_entry_type::predicate)>;

		// every entries doivent avoir le meme type pour chacuns. 

		/*static consteval extract_underlying_subtype() {

		}*/

		/************************************************/

		/*template<typename Tp>
		using extract_underlying_t = typename std::conditional_t<
			std::is_enum_v<Tp>,
			std::underlying_type_t<std::decay_t<Tp>>,
			std::type_identity<Tp>
		>;

		static_assert(
			((
				std::is_convertible_v<extract_underlying_t<decltype(std::declval<Entries>().source)>, integer_type>    &&
				std::is_convertible_v<extract_underlying_t<decltype(std::declval<Entries>().predicate)>, integer_type> &&
				std::is_convertible_v<extract_underlying_t<decltype(std::declval<Entries>().target)>, integer_type>
			) && ...),
			"message"
		);*/


		/************************************************/


		/*static_assert(
			((
				requires(Entries e) {
					requires std::constructible_from<integer_type, std::remove_cvref_t<decltype(e.source)>>;
					requires std::constructible_from<integer_type, std::remove_cvref_t<decltype(e.predicate)>>;
					requires std::constructible_from<integer_type, std::remove_cvref_t<decltype(e.target)>>;
				}
			) && ...),
			"message"
		);
*/


/*	static_assert(
		(std::is_convertible_v<
			std::conditional_t<
				std::is_enum_v<std::decay_t<decltype(Entries::source)>>,
				std::underlying_type_t<std::decay_t<decltype(Entries::source)>>,
				std::decay_t<decltype(Entries::source)>
			>
		, integer_type> && ...),

		"message"


	);*/

	/*static_assert(
		std::conditional_t<
			std::is_enum_v<row_type>,

		>

	);*/

	// static assert genre si il n'est pas convertible en int

	protected:
		static constexpr std::size_t row_maximum_size = []<typename... Entries>(std::tuple<Entries...>*) {
			return std::max(
				std::max({ static_cast<integer_type>(Entries::source)... }),
				std::max({ static_cast<integer_type>(Entries::target)... })
			) + 1;
		}(static_cast<configuration_tuple_t*>(nullptr));


		static constexpr std::size_t column_maximum_size = []<typename... Entries>(std::tuple<Entries...>*) {
			return std::max({ static_cast<integer_type>(Entries::predicate)... }) + 1;
		}(static_cast<configuration_tuple_t*>(nullptr));



	
#if 0
		static constexpr std::size_t row_maximum_size = std::max(
			std::max({ static_cast<integer_type>(Entries::source)... }),
			std::max({ static_cast<integer_type>(Entries::target)... })
		) + 1;

		static constexpr std::size_t column_maximum_size = std::max(
			{ static_cast<integer_type>(Entries::predicate)...
			}) + 1;
#endif

	public:

		// pour regrouper avec des functions genre isalpha, il faudrait 
		// une couche par dessus qui ajoute tous les char compris dans la 
		// fonction dans la table de la matrice, aussi simple que ca.

		[[nodiscard]] constexpr bool step(column_type predicate) {
			return dfa_.step(static_cast<integer_type>(predicate));
		}

		[[nodiscard]] constexpr row_type get_current_state()
			const noexcept(noexcept(dfa_.get_current()))
		{
			return static_cast<row_type>(dfa_.get_current());
		}

		[[nodiscard]] constexpr row_type get_previous_state()
			const noexcept(noexcept(dfa_.get_previous()))
		{
			return static_cast<row_type>(dfa_.get_previous());
		}

	public:
		void reset() noexcept(noexcept(dfa_.reset())) {
			dfa_.reset();
		}

	private:
		static consteval std::array<integer_type, row_maximum_size * column_maximum_size> generateDfaTable() {
			auto temp = []() {
				std::array<integer_type, row_maximum_size* column_maximum_size> arr;
				arr.fill(-1);
				return arr;
			}();

			[&]<typename... Entries>(std::tuple<Entries...>*) {
				((
					temp[static_cast<integer_type>(Entries::source) * column_maximum_size +
						static_cast<integer_type>(Entries::predicate)] = static_cast<integer_type>(Entries::target)
				), ...);
			}(static_cast<configuration_tuple_t*>(nullptr));

			return temp;
		}

	private:
		FlatMatrixDFA<row_maximum_size, column_maximum_size> dfa_{ generateDfaTable() };

	};




#if 0
	template<typename... Entries>
		//requires (std::is_same_v<int, Entries> || ...)
	struct StaticDFA  final { // genre configurable ou modularDfa
	protected:
		using integer_type = int;

		using first_entry_type = std::tuple_element_t<0, std::tuple<Entries...>>;

		using row_type = std::decay_t<decltype(first_entry_type::source)>; // genre remove ou decay
		using column_type = std::decay_t<decltype(first_entry_type::predicate)>;

		// every entries doivent avoir le meme type pour chacuns. 

		/*static consteval extract_underlying_subtype() {

		}*/

		/************************************************/

		/*template<typename Tp>
		using extract_underlying_t = typename std::conditional_t<
			std::is_enum_v<Tp>,
			std::underlying_type_t<std::decay_t<Tp>>,
			std::type_identity<Tp>
		>;

		static_assert(
			((
				std::is_convertible_v<extract_underlying_t<decltype(std::declval<Entries>().source)>, integer_type>    &&
				std::is_convertible_v<extract_underlying_t<decltype(std::declval<Entries>().predicate)>, integer_type> &&
				std::is_convertible_v<extract_underlying_t<decltype(std::declval<Entries>().target)>, integer_type>
			) && ...),
			"message"
		);*/


		/************************************************/


		/*static_assert(
			((
				requires(Entries e) {
					requires std::constructible_from<integer_type, std::remove_cvref_t<decltype(e.source)>>;
					requires std::constructible_from<integer_type, std::remove_cvref_t<decltype(e.predicate)>>;
					requires std::constructible_from<integer_type, std::remove_cvref_t<decltype(e.target)>>;
				}
			) && ...),
			"message"
		);
*/


	/*	static_assert(
			(std::is_convertible_v<
				std::conditional_t<
					std::is_enum_v<std::decay_t<decltype(Entries::source)>>,
					std::underlying_type_t<std::decay_t<decltype(Entries::source)>>,
					std::decay_t<decltype(Entries::source)>
				>
			, integer_type> && ...),

			"message"
			
			
		);*/

		/*static_assert(
			std::conditional_t<
				std::is_enum_v<row_type>,
				
			>
			
		);*/

		// static assert genre si il n'est pas convertible en int

	protected:
		static constexpr std::size_t row_maximum_size = std::max(
			std::max({ static_cast<integer_type>(Entries::source)... }),
			std::max({ static_cast<integer_type>(Entries::target)... })
		) + 1;

		static constexpr std::size_t column_maximum_size = std::max(
			{ static_cast<integer_type>(Entries::predicate)...
		}) + 1;

	public:
		
		// pour regrouper avec des functions genre isalpha, il faudrait 
		// une couche par dessus qui ajoute tous les char compris dans la 
		// fonction dans la table de la matrice, aussi simple que ca.

		[[nodiscard]] constexpr bool step(column_type predicate) {
			return dfa_.step(static_cast<integer_type>(predicate));
		}

		[[nodiscard]] constexpr row_type get_current_state()
			const noexcept(noexcept(dfa_.get_current()))
		{
			return static_cast<row_type>(dfa_.get_current());
		}

		[[nodiscard]] constexpr row_type get_previous_state()
			const noexcept(noexcept(dfa_.get_previous()))
		{
			return static_cast<row_type>(dfa_.get_previous());
		}

	public:
		void reset() noexcept(noexcept(dfa_.reset())) {
			dfa_.reset();
		}

	private:
		static consteval std::array<integer_type, row_maximum_size * column_maximum_size> generateDfaTable() {
			auto temp = []() {
				std::array<integer_type, row_maximum_size * column_maximum_size> arr;
				arr.fill(-1);
				return arr;
			}();

			((
				temp[static_cast<integer_type>(Entries::source) * column_maximum_size + static_cast<integer_type>(Entries::predicate)]
						= static_cast<integer_type>(Entries::target)
			), ...);

			return temp;
		}

	private:
		FlatMatrixDFA<row_maximum_size, column_maximum_size> dfa_{ generateDfaTable() };

	};
#endif
	// backup au cas ou

/***********************************************************************************/


	template<char... c>
	struct charset {
		// principalement pour le Lexer mais peut aussi servir a quelque pars d'autre
	};




	template</*ConfigurationEntryConcept*/ typename... Entries>
	struct generate_expanded_dfa_config { // genre Adapter

		using entry_tuple_t = std::tuple<make_tuple_from_config_entry_t<Entries>...>;


		static constexpr bool respects_all_conditions = []<typename... entry_tuples>(std::tuple<entry_tuples...>*) {
			return ((
				std::tuple_size_v<entry_tuples> == 4 &&
				std::is_same_v<
					decltype(std::tuple_element_t<1, entry_tuples>::value),
					decltype(std::tuple_element_t<3, entry_tuples>::value)
				>

				// faudrait mettre genre faut que celui du milieu soit le truc qui contient les lettres

			) && ...);
		}(static_cast<entry_tuple_t*>(nullptr));

		static_assert(respects_all_conditions, "mettre un message ici");

		using generated_tuple_t = decltype([]<typename Tuple>(std::type_identity<Tuple>) {
			return[]<typename... Ts>(std::type_identity<std::tuple<Ts...>>) {
				return std::tuple_cat(
					[]<typename SingleEntry>() {
						using CurrentCharset = std::tuple_element_t<2, SingleEntry>;

						return[]<char... Characters>(charset<Characters...>*) {
							return std::tuple<
								GenericConfigurationEntry<
									std::tuple_element_t<0, SingleEntry>::value,
									std::tuple_element_t<1, SingleEntry>,
									nttp_to_type<Characters>,
									std::tuple_element_t<3, SingleEntry>
								>...
							>{};
						}(static_cast<CurrentCharset*>(nullptr));
					}.template operator()<Ts>()...
				);
			}(std::type_identity<Tuple>{});
		}(std::type_identity<entry_tuple_t>{}));


		using type = decltype([]<typename... entries>(std::tuple<entries...>*) {
			return StaticDfaTransitions<entries...>{};
		}(static_cast<generated_tuple_t*>(nullptr)));

	};

	template<typename... Entries>
	using generate_expanded_dfa_config_t = typename generate_expanded_dfa_config<Entries...>::type;



/***********************************************************************************/



	using charset_alpha = charset<
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
		'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
	>;

	using charset_digits = charset<
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
	>;

	using charset_alphanumeric = charset<
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
		'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
		'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
	>;


	using charset_isdelimiter_pure = charset<
		'.', '@'
	>;


	using charset_isoperator_pure = charset<
		'+', '-', '*', '/', '%', '=', '!', '&', '|', '^', '~', '?'
	>;

	using charset_iswhitespace = charset<
		' ', '\t', '\r'
	>;


	using charset_ispreprocessor = charset<
		'#'
	>;


	using charset_isoperator = charset<
		'+', '-', '*', '/', '%', '=', '<', '>', '!', '&', '|', '^', '~', '?'
	>;



/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/


	template<bool Condition, typename Target, typename Predicate>
	struct ConfigurationRule {
		static constexpr bool condition = Condition;

		using target = Target;
		using predicate = Predicate;
	};

	template<typename Target, typename Predicate>
	using ENABLED_old = ConfigurationRule<true, Target, Predicate>;

	template<typename Target, typename Predicate>
	using DISABLED_old = ConfigurationRule<false, Target, Predicate>;

	template<auto Condition, typename Target, typename Predicate>
	using CONDITIONAL_old = ConfigurationRule<Condition, Target, Predicate>;


/********************************************************************************/

	// pas certain que ca soit encore utilisé
	template<typename T>
	concept SystemConfigurationConcept = requires {
		typename T::as_tuple;
	} && (requires { typename T::Tail; } || std::same_as<typename T::as_tuple, std::tuple<>>);


	template<typename... Ts>
	struct SystemConfiguration;

	template<>
	struct SystemConfiguration<> {
		using as_tuple = std::tuple<>;
	};

	template<typename First, typename... Rest> // must be ConfigurationRule
	struct SystemConfiguration<First, Rest...> {
		using Tail = typename SystemConfiguration<Rest...>::as_tuple;

		using as_tuple = std::conditional_t<
			First::condition,
			decltype(std::tuple_cat(std::tuple<First>{}, Tail{})),
			Tail
		>;
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

	template<typename T>
	struct is_keyword_matching_policy_specialization : std::false_type {};

	template<fixed_string... Principles>
	struct is_keyword_matching_policy_specialization<KeywordMatchingPolicy<Principles...>> : std::true_type {};

	template<typename T>
	concept is_keyword_matching_policy = is_keyword_matching_policy_specialization<T>::value;


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


	enum struct TokenKind {
		Identifier = 0,

		KeywordOpaque = 10,
		KeywordType = 11,
		KeywordQualifier = 12,
		KeywordSpecifier = 13,
		KeywordModifier = 14,
		KeywordAlignment = 15,
		KeywordControl = 16,
		KeywordAccess = 17,

		DelimiterOpaque = 20,
		DelimiterColon = 21,
		DelimiterSemicolon = 22,
		DelimiterComma = 23,

		DelimiterRParen = 30,
		DelimiterLParen = 31,
		DelimiterLCurly = 32,
		DelimiterRCurly = 33,
		DelimiterRSquare = 34,
		DelimiterLSquare = 35,
		DelimiterRAngle = 36,
		DelimiterLAngle = 37,

		Preprocessor = 40,
		Operator = 41,
		Number = 42,
		Whitespace = 43,
		Newline = 44,

		Invalid = 90,
		Unknown = 91
	};


	[[nodiscard]] constexpr std::string_view TokenKind_to_string(TokenKind kind) noexcept {
		switch (kind) {
			case TokenKind::Identifier:       return "Identifier";
			case TokenKind::KeywordOpaque:    return "KeywordOpaque";
			case TokenKind::KeywordType:      return "KeywordType";
			case TokenKind::KeywordQualifier: return "KeywordQualifier";
			case TokenKind::KeywordSpecifier: return "KeywordSpecifier";
			case TokenKind::KeywordModifier:  return "KeywordModifier";
			case TokenKind::KeywordAlignment: return "KeywordAlignment";
			case TokenKind::KeywordControl:   return "KeywordControl";
			case TokenKind::KeywordAccess:    return "KeywordAccess";
			case TokenKind::DelimiterOpaque:  return "DelimiterOpaque";
			case TokenKind::DelimiterColon:   return "DelimiterColon";
			case TokenKind::DelimiterSemicolon: return "DelimiterSemicolon";
			case TokenKind::DelimiterComma:   return "DelimiterComma";
			case TokenKind::DelimiterRParen:  return "DelimiterRParen";
			case TokenKind::DelimiterLParen:  return "DelimiterLParen";
			case TokenKind::DelimiterLCurly:  return "DelimiterLCurly";
			case TokenKind::DelimiterRCurly:  return "DelimiterRCurly";
			case TokenKind::DelimiterRSquare: return "DelimiterRSquare";
			case TokenKind::DelimiterLSquare: return "DelimiterLSquare";
			case TokenKind::DelimiterRAngle:  return "DelimiterRAngle";
			case TokenKind::DelimiterLAngle:  return "DelimiterLAngle";
			case TokenKind::Preprocessor:     return "Preprocessor";
			case TokenKind::Operator:         return "Operator";
			case TokenKind::Number:           return "Number";
			case TokenKind::Whitespace:       return "Whitespace";
			case TokenKind::Newline:          return "Newline";
			case TokenKind::Invalid:          return "Invalid";
			case TokenKind::Unknown:          return "Unknown";
		}
		return "Unknown_TokenKind_Value";
	}


/********************************************************************************/

#if 0
	template<typename T>
	concept IsaKeywordMatchingPolicy =
		requires(std::string_view sv) {
			{ T::matches(sv) } -> std::same_as<bool>;
	};


/********************************************************************************/


	export template<typename Predicate, auto Corresponding>
	struct TokenClassifierContext final {
		using predicate_type = Predicate;
		static constexpr TokenKind corresponding = Corresponding;
	};

	template<typename T>
	concept is_token_classifier_context = requires {
		typename T::predicate_type;
		T::corresponding;
	};


/********************************************************************************/


	export template<typename Configuration> // POUR MOCK TEMPORAIRE
		struct TokenKeywordClassifier final {};
#endif











/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/




	template<typename entry_tuple>
	struct TokenKeywordCategorizerConfigurationSchema {
		using first_t = std::decay_t<std::tuple_element_t<0, entry_tuple>>;
		using second_t = std::decay_t<std::tuple_element_t<1, entry_tuple>>;

		static constexpr bool valid =
			requires {
				requires std::tuple_size_v<entry_tuple> == 2;

				requires is_keyword_matching_policy<first_t>;
				requires std::is_same_v<std::decay_t<decltype(second_t::value)>, TokenKind>;
			};
	};

	template<typename entry_tuple>
	struct TokenKeywordCategorizerConfigurationModel final {
		using predicate = std::tuple_element_t<0, entry_tuple>;
		static constexpr TokenKind corresponding = std::tuple_element_t<1, entry_tuple>::value;
	};

	template<typename... Entries>
	using TokenKeywordCategorizerConfiguration = new_ConfigurationSystem<
		TokenKeywordCategorizerConfigurationSchema,
		TokenKeywordCategorizerConfigurationModel,
		Entries...
	>;

	template <typename T>
	constexpr bool is_token_keyword_categorizer_config_v = false;

	template <typename... Entries>
	constexpr bool is_token_keyword_categorizer_config_v<TokenKeywordCategorizerConfiguration<Entries...>> = true;

	template <typename T>
	concept is_token_keyword_categorizer_configuration = is_token_keyword_categorizer_config_v<T>;

	// ca se répète souvent, y'aurais peut-être moyen de factoriser tout cela de facon a rendre la chose plus lisible




	export template<is_token_keyword_categorizer_configuration Configuration>
	struct TokenKeywordCategorizer final {
	protected:
		using configuration_tuple_t = typename Configuration::type;

		static constexpr bool is_nothrow_config = []<typename... Ts>(std::type_identity<std::tuple<Ts...>>) {
			return (noexcept(Ts::predicate::matches(std::string_view{})) && ...);
		}(std::type_identity<configuration_tuple_t>{});

	public:
		[[nodiscard]] static constexpr TokenKind transform(std::string_view sv) noexcept(is_nothrow_config) {
			return [&]<typename... Ts>(std::type_identity<std::tuple<Ts...>>) {
				TokenKind result = TokenKind::Unknown;
				(((Ts::predicate::matches(sv)) && (result = Ts::corresponding, true)), ...);

				return result;
			}(std::type_identity<configuration_tuple_t>{});
		}
	};

#if 0
	export template<is_token_classifier_context... Contexts>
	struct TokenKeywordCategorizer final { // re-categorizer?
		private:
			template<typename Current, typename... Remaining>
			[[nodiscard]] static constexpr TokenKind evaluate_recursively(std::string_view sv)
				noexcept(noexcept(Current::predicate_type::matches(sv)))
			{
				if (Current::predicate_type::matches(sv))
					return Current::corresponding;

				if constexpr (sizeof...(Remaining) == 0)
					return TokenKind::Unknown;
				else
					return evaluate_recursively<Remaining...>(sv);
			}

		public:
			[[nodiscard]] static constexpr TokenKind transform(std::string_view sv)
				noexcept(noexcept(evaluate_recursively<Contexts...>(sv)))
			{
				return evaluate_recursively<Contexts...>(sv);
			}
	};
#endif // petite backup


/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/
/***********************************************************************************/


	struct SourceLocation {
		std::size_t line;   // 8 bytes, std::uint16_t???
		std::size_t column; // 8 bytes
	}; 

	struct Token {
		//Token(TokenType k, const char)



		TokenKind kind;


		//const char* lexeme; // string interné

		std::string_view lexeme; // TEMPORAIRE

		SourceLocation location;
	};


/********************************************************************************/


	struct CharReader { // changer le nom genre CharStreamReader
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

		[[nodiscard]] char peek() const noexcept { // constexpr?
			return *cursor_;
		}

		// fonction peek pour regarder le char actuel

		// PEUT-ÊTRE PLUTOT FAIRE GENRE UN FOR EACH EXTERNE ET APELLER LE TRUC GENRE CHARSTREAM <--------------------------- !!!!!!!!!!!!!!


		template<typename F>
		void consume(F&& task) {


			for (; *cursor_ != '\0'; ++cursor_) {
				task(cursor_);
			}


		}




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

		SourceLocation& current_location() { // ou get_location (probablement mieux)
			return current;
		}




	private:
		SourceLocation current{}; // current_???

	};


/********************************************************************************/

	// DEVRAIS DEVENIR GENRE UNE CONFIGURATION PAR POLICY OU RULES, QUELQUE CHOSE DU GENRE.

#if 0
	constexpr TokenKind state_to_token(LexState _) {
		switch (_) {
		case LexState::Identifier: return TokenKind::Identifier;
		case LexState::DelimiterOpaque: return TokenKind::DelimiterOpaque;

		case LexState::Operator: return TokenKind::Operator;

		case LexState::DelimiterColon: return TokenKind::DelimiterColon;
		case LexState::DelimiterSemi: return TokenKind::DelimiterSemicolon;
		case LexState::DelimiterComma: return TokenKind::DelimiterComma;

		case LexState::DelimiterLParen: return TokenKind::DelimiterLParen;
		case LexState::DelimiterRParen: return TokenKind::DelimiterRParen;

		case LexState::DelimiterLCurly: return TokenKind::DelimiterLCurly;
		case LexState::DelimiterRCurly: return TokenKind::DelimiterRCurly;

		case LexState::DelimiterLSquare: return TokenKind::DelimiterLSquare;
		case LexState::DelimiterRSquare: return TokenKind::DelimiterRSquare;

		case LexState::DelimiterLAngle: return TokenKind::DelimiterLAngle;
		case LexState::DelimiterRAngle: return TokenKind::DelimiterRAngle;

		case LexState::Preprocessor: return TokenKind::Preprocessor;
		case LexState::Newline: return TokenKind::Newline;

		case LexState::Number: return TokenKind::Number;
		case LexState::Invalid: return TokenKind::Unknown;

		default: return TokenKind::Invalid;
		}
	}
#endif

#if 0
	// deprecated
	template<auto Source, auto Target>
	struct EnumMapperEntry {
		static constexpr auto source = Source;
		static constexpr auto target = Target;
	};
#endif







	template <typename T, typename... AllowedTypes>
	inline constexpr bool is_any_of_v = (std::is_same_v<T, AllowedTypes> || ...);


















	template<typename entry_tuple>
	struct EnumMapperConfigurationSchema {
		using first_t = std::decay_t<std::tuple_element_t<0, entry_tuple>>;
		using second_t = std::decay_t<std::tuple_element_t<1, entry_tuple>>;

		static constexpr bool valid =
			requires {
				requires std::tuple_size_v<entry_tuple> == 2;

				//requires std::same_as<first_t, nttp_to_type>; // faut faire un trait
				//requires std::same_as<second_t, nttp_to_type>;

				requires !std::same_as<decltype(first_t::value), decltype(second_t::value)>;
				//requires first_t::value != second_t::value; // pas d'operateur == entre les deux...
		};
	};

	template<typename entry_tuple>
	struct EnumMapperConfigurationModel final {
		static constexpr auto source = std::tuple_element_t<0, entry_tuple>::value;
		static constexpr auto target = std::tuple_element_t<1, entry_tuple>::value;
	};

	template<typename... Entries>
	using EnumMapperConfiguration = new_ConfigurationSystem<
		EnumMapperConfigurationSchema,
		EnumMapperConfigurationModel,
		Entries...
	>;

	template <typename T>
	constexpr bool is_enum_mapper_v = false;

	template <typename... Entries>
	constexpr bool is_enum_mapper_v<EnumMapperConfiguration<Entries...>> = true;

	template <typename T>
	concept is_enum_mapper_configuration = is_enum_mapper_v<T>;



	

	template<is_enum_mapper_configuration Configuration>
	struct EnumMapper {
	protected:
		using configuration_tuple_t = typename Configuration::type;
		using rule_model_t = std::tuple_element_t<0, configuration_tuple_t>;

		using first_enum_t = std::decay_t<decltype(rule_model_t::source)>;
		using second_enum_t = std::decay_t<decltype(rule_model_t::target)>;

	protected:
		static constexpr bool is_valid = []<typename... Ts>(std::type_identity<std::tuple<Ts...>>) {
			return ((
				is_any_of_v<std::decay_t<decltype(Ts::source)>, first_enum_t, second_enum_t> &&
				is_any_of_v<std::decay_t<decltype(Ts::target)>, first_enum_t, second_enum_t>
			) && ...);
		}(std::type_identity<configuration_tuple_t>{});

		static_assert(is_valid, "EnumMapper error: Too many distinct state types detected. A maximum of two are allowed.");

	protected:
		template<typename Tp>
		static constexpr bool is_nothrow_find_target = []<typename... Ts>(std::type_identity<std::tuple<Ts...>>) {
			return (noexcept(std::declval<Tp>() == Ts::source) && ...);
		}(std::type_identity<configuration_tuple_t>{});

		template<typename Tp>
		static constexpr bool is_nothrow_find_source = []<typename... Ts>(std::type_identity<std::tuple<Ts...>>) {
			return (noexcept(std::declval<Tp>() == Ts::target) && ...);
		}(std::type_identity<configuration_tuple_t>{});

	public:
		template<typename Tp>
		[[nodiscard]] static constexpr auto find_target(Tp&& source) 
			noexcept(is_nothrow_find_target<Tp>)
		{
			return [&]<typename... Ts>(std::type_identity<std::tuple<Ts...>>) {
				using TargetType = std::common_type_t<decltype(Ts::target)...>;
				std::optional<TargetType> result;

				(((source == Ts::source) && (result = Ts::target, true)), ...);
				if (!result) throw std::runtime_error("Target not found");

				return result.value();
			}(std::type_identity<configuration_tuple_t>{});
		}

		template<typename Tp>
		[[nodiscard]] static constexpr auto find_source(Tp&& target)
			noexcept(is_nothrow_find_source<Tp>)
		{
			return[&]<typename... Ts>(std::type_identity<std::tuple<Ts...>>) {
				using TargetType = std::common_type_t<decltype(Ts::source)...>;
				std::optional<TargetType> result;

				(((target == Ts::target) && (result = Ts::source, true)), ...);
				if (!result) throw std::runtime_error("Source not found");

				return result.value();
			}(std::type_identity<configuration_tuple_t>{});
		}
	};


/********************************************************************************/


	// LexicalAnalyzer<LexingAutomaton, KeywordClassifier> lexer;

	// template reader & tracker + probablement le automaton

	template<
		//typename Reader, typename Tracker,
		typename LexingAutomaton, typename Mapper, typename Recategoriser
	>
	struct Lexer {


		// peut etre mettre soit les automaton et classifier
		// ou mettre le tracker et le reader, à voir...

		template <typename Reader, typename Tracker>
		[[nodiscard]] std::vector<Token> tokenize(std::string_view source) {

			const char* begin = source.data();
			const char* end = source.data() + source.size();

			const char* current = begin;
			const char* current_seg_start = current;


			Reader reader{ begin, end }; // le call Stream?
			Tracker tracker{};


			std::vector<Token> tokens; // utiliser un allocator
			tokens.reserve(575122); // 1024 1 << 1024 


			reader.consume([&](const char* current) {

				char c = *current;

				//std::cout << "current lexing char: " << c << "\n";

				SourceLocation cur_location = tracker.update(c);
				bool success = automaton.step(c);

				//std::cout << "\tcurrent state: " << static_cast<int>(automaton.get_current_state()) << "\n";
				//std::cout << "\tprevious state: " << static_cast<int>(automaton.get_previous_state()) << "\n";

				if (!success) [[unlikely]] {
					auto final_state = automaton.get_previous_state();

					if (final_state != LexState::Whitespace && // couplage obligatoire même si c'est moyen moyen
						final_state != LexState::Newline) {

						tokens.emplace_back(
							mapper.find_target(final_state),
							std::string_view(current_seg_start, static_cast<std::ptrdiff_t>(current - current_seg_start)),
							cur_location
						);

						//std::cout << "[" << std::string_view(current_seg_start, static_cast<std::ptrdiff_t>(current - current_seg_start)) << "]\n";
					}

					automaton.reset();

					automaton.step(c);
					current_seg_start = current;

					// FAUT GERER LE CASE DU MOT FINAL MAIS J'AI LA FLEME DONC TODO!!!!!!!!!!
				}
			});


			for (auto& t : tokens) { // pas le plus optimal mais reste le plus générique
				auto result = recategoriser.transform(t.lexeme);
				if (result == TokenKind::Invalid) [[unlikely]] t.kind = result;
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


		[[no_unique_address]] LexingAutomaton automaton{}; // DEVRAIT PROBABLEMENT SE CONSOMMER COMME LES AUTRES, STATEFUL
		[[no_unique_address]] Recategoriser recategoriser{}; // module séparé peut-être

		[[no_unique_address]] Mapper mapper{};
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












[[nodiscard]] std::string generate_lexer_stress_test() {
	std::ostringstream oss;

	// 1. Un en-tête complexe pour tester les préprocesseurs, délimiteurs et blancs
	oss << "#include <iostream>\n";
	oss << "#include <vector>\n\n";
	oss << "/* Test de commentaires bloc et opaque */\n";
	oss << "namespace test_lexer::core {\n";
	oss << "    public constexpr inline auto process_data() noexcept -> int {\n";
	oss << "        int accumulator = 12345;\n";
	oss << "        bool is_valid = true;\n";
	oss << "        if (accumulator >= 500 && is_valid) {\n";
	oss << "            accumulator = accumulator * 2 + (42 - 7);\n";
	oss << "        }\n";
	oss << "        return accumulator;\n";
	oss << "    }\n";
	oss << "}\n\n";

	// 2. Génération d'une boucle massive pour saturer le DFA et le Recategoriser
	// On génère 5000 blocs répétitifs mais variés
	for (int i = 0; i < 5000; ++i) {
		oss << "struct TokenClassifierContext" << i << " {\n";
		oss << "    static constexpr TokenKind value = TokenKind::KeywordAccess;\n";
		oss << "    using policy = AccessKeywordMatchingPolicy;\n";
		oss << "    const volatile int local_flag = " << i << ";\n";
		oss << "    // Alternance de mots-clés de contrôle et modificateurs\n";
		oss << "    void check(bool condition) {\n";
		oss << "        if (condition) { return; } else { do_nothing(); }\n";
		oss << "        alignas(8) char buffer[256];\n";
		oss << "        int* ptr = &local_flag;\n";
		oss << "        if (*ptr == " << i << " && true || false) {\n";
		oss << "            auto list = { 1, 2, 3, 4, 5 };\n";
		oss << "        }\n";
		oss << "    }\n";
		oss << "};\n\n";
	}

	// 3. Un final brut avec des enchaînements rapides d'opérateurs et d'invalides
	oss << "hello world, j'aime le c++\n retour a la ligne\n";
	oss << "&&& *** ::: ;;; ,,, <<< >>> {{}} [[]] (()) \n";
	oss << "0 1 23 456 7890 1234567890\n";

	return oss.str();
}






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

		std::string content = "hello world, jaime le c++\n retour a la ligne";

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

	/*	using LexingAutomaton = CompileTimeDfa<15, 15,

			dfa_transition<LexState::Start, '\n', LexState::Newline>
		>;*/










#if 0
		std::vector<Token> tokens = lexer.tokenize<CharReader, PositionTracker>(content);

		std::cout << "Tokens: \n";
		for (auto const& t : tokens) {
			std::cout << '[' << t.lexeme << ']';
		}
#endif






		using System = ModularSystem<
			SystemConfiguration<
				ENABLED_old		<MockModuleA, MockModuleB>,
				DISABLED_old	<CharReader, PositionTracker>

				//CONDITIONAL_old <(sizeof(content) != 0), ContextComposer, CompilationUnit>
			>
		>;

		/*using ModularTokenKwrdClassifier = TokenKeywordClassifier<
			SystemConfiguration<
				ENABLED <>
			>
		>;*/


		StaticMatrix<3, 3> m{ // genre laisser un commentaire après SystemConfiguration genre /* transition configuration */
			0, 1, 2,
			3, 0, 0,
			0, 4, 0
		};

		std::cout << m.at(2, 1) << "\n";



		auto state = LexState::Start;
		auto arg = '\n';

		//std::cout << "[IntegerBacktrackingDFA] result: " << dfa.m_.at(static_cast<int>(state), static_cast<int>(arg)) << "\n";



		/********************/


		/*using LexingDFA = StaticDFA<
			dfa_transition<LexState::Start, '\n', LexState::Newline>
		>;*/


		using LexingDFA = StaticDFA<
			StaticDfaTransitions<
				GenericConfigurationEntry<true, nttp_to_type<LexState::Start>, nttp_to_type<'\n'>, nttp_to_type<LexState::Newline>>
			>
		>;

		LexingDFA lexdfa{};

		std::cout << "[LexingDFA -> before] result: " << static_cast<int>(lexdfa.get_current_state()) << "\n";

		lexdfa.step('\t');

		std::cout << "[LexingDFA -> after] result: " << static_cast<int>(lexdfa.get_current_state()) << "\n";



		//AdapterDFA adapter;




		//using MySystem = ConfigurableSystem<
		//	SystemConfiguration<
		//		GenericConfigurationEntry	<true, CharReader, PositionTracker>,
		//		GenericConfigurationEntry	<true, MockModuleC, MockModuleD>
		//	>
		//>;

		//MySystem sys;

		//std::cout << "\n";
		//sys.print_ruleset();
		//std::cout << "\n\n";







		using res_t = LambdaSystemConfiguration<
			GenericConfigurationEntry<true, MockModuleA, PositionTracker>,
			GenericConfigurationEntry<true, MockModuleB, PositionTracker>
		>::type;


		std::apply([](auto... args) {
			((
				std::cout << typeid(std::remove_cvref_t<decltype(args)>).name() << "\n",

				std::cout << "  \trule_t: " << typeid(typename decltype(args)::rule_t).name() << "\n",
				std::cout << "  \tbehavior_t: " << typeid(typename decltype(args)::behavior_t).name() << "\n\n"
			), ...);
		}, res_t{});





		//using LexingAutomaton = StaticDFA<
		//	StaticDfaTransitions<
		//		ENABLED <nttp_to_type<LexState::Start>, nttp_to_type<'\n'>, nttp_to_type<LexState::Newline>>
		//	>
		//>;





#if !defined(__INTELLISENSE__)
		using LexingAutomaton__final = StaticDFA<
			generate_expanded_dfa_config_t<
				ENABLED<nttp_to_type<LexState::Start>, charset_alpha, nttp_to_type<LexState::Identifier>>,
				ENABLED<nttp_to_type<LexState::Start>, charset_digits, nttp_to_type<LexState::Number>>,

				ENABLED<nttp_to_type<LexState::Start>, charset<':'>, nttp_to_type<LexState::DelimiterColon>>,
				ENABLED<nttp_to_type<LexState::Start>, charset<';'>, nttp_to_type<LexState::DelimiterSemi>>,
				ENABLED<nttp_to_type<LexState::Start>, charset<','>, nttp_to_type<LexState::DelimiterComma>>,

				ENABLED<nttp_to_type<LexState::Start>, charset<'('>, nttp_to_type<LexState::DelimiterLParen>>,
				ENABLED<nttp_to_type<LexState::Start>, charset<')'>, nttp_to_type<LexState::DelimiterRParen>>,

				ENABLED<nttp_to_type<LexState::Start>, charset<'{'>, nttp_to_type<LexState::DelimiterLCurly>>,
				ENABLED<nttp_to_type<LexState::Start>, charset<'}'>, nttp_to_type<LexState::DelimiterRCurly>>,

				ENABLED<nttp_to_type<LexState::Start>, charset<'['>, nttp_to_type<LexState::DelimiterLSquare>>,
				ENABLED<nttp_to_type<LexState::Start>, charset<']'>, nttp_to_type<LexState::DelimiterRSquare>>,

				ENABLED<nttp_to_type<LexState::Start>, charset<'<'>, nttp_to_type<LexState::DelimiterLAngle>>,
				ENABLED<nttp_to_type<LexState::Start>, charset<'>'>, nttp_to_type<LexState::DelimiterRAngle>>,

				ENABLED<nttp_to_type<LexState::Start>, charset<'\n'>, nttp_to_type<LexState::Newline>>,
				ENABLED<nttp_to_type<LexState::Start>, charset_isdelimiter_pure, nttp_to_type<LexState::DelimiterOpaque>>,
				ENABLED<nttp_to_type<LexState::Start>, charset_isoperator_pure, nttp_to_type<LexState::Operator>>,

				ENABLED<nttp_to_type<LexState::Start>, charset_iswhitespace, nttp_to_type<LexState::Whitespace>>,
				ENABLED<nttp_to_type<LexState::Start>, charset_ispreprocessor, nttp_to_type<LexState::Preprocessor>>,

				ENABLED<nttp_to_type<LexState::Preprocessor>, charset_alpha, nttp_to_type<LexState::Preprocessor>>,
				ENABLED<nttp_to_type<LexState::Identifier>, charset_alphanumeric, nttp_to_type<LexState::Identifier>>,

				ENABLED<nttp_to_type<LexState::Number>, charset_digits, nttp_to_type<LexState::Number>>,
				ENABLED<nttp_to_type<LexState::Operator>, charset_isoperator, nttp_to_type<LexState::Operator>>,

				ENABLED<nttp_to_type<LexState::DelimiterColon>, charset<':'>, nttp_to_type<LexState::DelimiterColon>>,
				ENABLED<nttp_to_type<LexState::Whitespace>, charset_iswhitespace, nttp_to_type<LexState::Whitespace>>
			>
		>;
#else
		using LexingAutomaton__final = StaticDFA<
			StaticDfaTransitions<
				ENABLED <nttp_to_type<LexState::Start>, nttp_to_type<'\n'>, nttp_to_type<LexState::Newline>>
			>
		>;
#endif


		//case LexState::Operator: return TokenKind::Operator;


		//using MyEnumMapper = EnumMapper<
		//	EnumMapperConfiguration<
		//		ENABLED		<nttp_to_type<LexState::Identifier>, nttp_to_type<TokenKind::Identifier>>,
		//		DISABLED	<nttp_to_type<LexState::Operator>,   nttp_to_type<TokenKind::Operator>, int>
		//	>
		//>;





		using LexStateToTokenMapper__final = EnumMapper<
			EnumMapperConfiguration<
				ENABLED		<nttp_to_type<LexState::Identifier>,	   nttp_to_type<TokenKind::Identifier>>,
				ENABLED		<nttp_to_type<LexState::DelimiterOpaque>,  nttp_to_type<TokenKind::DelimiterOpaque>>,
				ENABLED		<nttp_to_type<LexState::Operator>,		   nttp_to_type<TokenKind::Operator>>,

				ENABLED		<nttp_to_type<LexState::DelimiterColon>,   nttp_to_type<TokenKind::DelimiterColon>>,
				ENABLED		<nttp_to_type<LexState::DelimiterSemi>,	   nttp_to_type<TokenKind::DelimiterSemicolon>>,
				ENABLED		<nttp_to_type<LexState::DelimiterComma>,   nttp_to_type<TokenKind::DelimiterComma>>,

				ENABLED		<nttp_to_type<LexState::DelimiterLParen>,  nttp_to_type<TokenKind::DelimiterLParen>> ,
				ENABLED		<nttp_to_type<LexState::DelimiterRParen>,  nttp_to_type<TokenKind::DelimiterRParen>> ,

				ENABLED		<nttp_to_type<LexState::DelimiterLCurly>,  nttp_to_type<TokenKind::DelimiterLCurly>>,
				ENABLED		<nttp_to_type<LexState::DelimiterRCurly>,  nttp_to_type<TokenKind::DelimiterRCurly>>,

				ENABLED		<nttp_to_type<LexState::DelimiterLSquare>, nttp_to_type<TokenKind::DelimiterLSquare>>,
				ENABLED		<nttp_to_type<LexState::DelimiterRSquare>, nttp_to_type<TokenKind::DelimiterRSquare>>,

				ENABLED		<nttp_to_type<LexState::DelimiterLAngle>,  nttp_to_type<TokenKind::DelimiterLAngle>>,
				ENABLED		<nttp_to_type<LexState::DelimiterRAngle>,  nttp_to_type<TokenKind::DelimiterRAngle>>,

				ENABLED		<nttp_to_type<LexState::Preprocessor>,	   nttp_to_type<TokenKind::Preprocessor>>,
				ENABLED		<nttp_to_type<LexState::Newline>,		   nttp_to_type<TokenKind::Newline>>,
				ENABLED		<nttp_to_type<LexState::Number>,		   nttp_to_type<TokenKind::Number>>,

				ENABLED		<nttp_to_type<LexState::Invalid>,		   nttp_to_type<TokenKind::Unknown>>
			>
		>;

		using TokenKwrdCategorizer__final = TokenKeywordCategorizer<
			TokenKeywordCategorizerConfiguration<
				ENABLED		<AccessKeywordMatchingPolicy,    nttp_to_type<TokenKind::KeywordAccess>>,
				ENABLED		<AlignmentKeywordMatchingPolicy, nttp_to_type<TokenKind::KeywordAlignment>>,
				ENABLED		<ControlKeywordMatchingPolicy,   nttp_to_type<TokenKind::KeywordControl>>,
				ENABLED		<ModifierKeywordMatchingPolicy,  nttp_to_type<TokenKind::KeywordModifier>>,
				ENABLED		<QualifierKeywordMatchingPolicy, nttp_to_type<TokenKind::KeywordQualifier>>,
				ENABLED		<SpecifierKeywordMatchingPolicy, nttp_to_type<TokenKind::KeywordSpecifier>>,
				ENABLED		<TypeKeywordMatchingPolicy,      nttp_to_type<TokenKind::KeywordType>>
			>
		>;



		LexStateToTokenMapper__final mapper;

		std::cout << "mapping result: " << static_cast<int>(mapper.find_target(LexState::Identifier)) << "\n";
		 




		using LexicalAnalyzer = Lexer<LexingAutomaton__final, LexStateToTokenMapper__final, TokenKwrdCategorizer__final>;


		std::string content = generate_lexer_stress_test();

		
		LexicalAnalyzer lexer; 

		auto start = std::chrono::steady_clock::now();


		std::vector<Token> tokens = lexer.tokenize<CharReader, PositionTracker>(content);


		auto endA = std::chrono::steady_clock::now();
		auto durationA = std::chrono::duration_cast<std::chrono::nanoseconds>(endA - start);
		std::cout << "[Tokenization] " << durationA.count() << " ns\n";


		//std::cout << "Tokens: \n";
		//for (auto const& t : tokens) {
		//	std::cout << " -> lexeme: " << t.lexeme << "\n";
		//	std::cout << "    kind: " << TokenKind_to_string(t.kind) << "\n";
		//	std::cout << "    pos: (" << t.location.column << ", " << t.location.line << ")\n\n";

		//	//std::cout << '[' << t.lexeme << ']';
		//}



		using test_configuration_static_dfa = generate_expanded_dfa_config_t<
			GenericConfigurationEntry<true, nttp_to_type<LexState::Start>, charset<'a', 'b', 'c'>, nttp_to_type<LexState::Newline>>

		>;


		//using FinalDfa = StaticDFA<
		//	generate_expanded_dfa_config_t<
		//		GenericConfigurationEntry<true, nttp_to_type<LexState::Start>, charset<'a', 'b', 'c'>, nttp_to_type<LexState::Newline>>
		//	>
		//>;   























		LexingAutomaton__final dfaf{};


		std::cout << "[dfaf -> before] result: " << static_cast<int>(dfaf.get_current_state()) << "\n";

		dfaf.step('d');

		std::cout << "[dfaf -> after] result: " << static_cast<int>(dfaf.get_current_state()) << "\n";





		std::cout << "Generation result: " << type_name<test_configuration_static_dfa>() << "\n";



	}

}
