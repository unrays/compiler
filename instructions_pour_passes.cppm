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
using TokenType = int;

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
	struct StreamSegmentationStrategy {
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

struct Token {
	TokenType type;

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


}
