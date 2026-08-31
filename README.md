## `instructions_pour_passes.cppm` - read this file to explore the code

> The project was originally intended to be a compiler. However, due to a lack of time and motivation, I decided to focus on my personal life and starting university instead of burning myself out trying to finish this behemoth. I studied compiler theory extensively, understood the pitfalls and the right approaches, and I was, and still am, ready to code a compiler from scratch. Yet, it is a titanic task; working alone, I find it hard to imagine pulling it all off, given the gargantuan architecture I had begun to implement. Perhaps one day I will return to complete it, but in the meantime, here is what I have. I really enjoyed writing this code, though the metaprogramming complexity makes it difficult to make progress. I am proud of it, which is why I am sharing it with you. This code was written during the summer between finishing my college-level programming studies and starting the same program at Université Laval.

> **To sum up, it is an unfinished project that embodies both ambition and exhaustion. Only the lexer is complete; the entire compiler structure is already in place, but the remaining components still need to be implemented. As for the code itself, it represents the pinnacle of what I, at twenty years old, am capable of producing in terms of architecture, metaprogramming, and low-level optimization. I have made significant progress over the past year, as evidenced by my GitHub projects, and I am very proud of this new building block I am adding to the edifice of my career.**


---


<details>
<summary>Expand to see the complete compile-time lexer assembly and DFA configuration</summary>

```cpp
#if !defined(__INTELLISENSE__)
using LexingAutomaton__final = StaticDFA<
	generate_expanded_dfa_config_t<
		ENABLED  <nttp_to_type<LexState::Start>, charset_alpha, nttp_to_type<LexState::Identifier>>,
		ENABLED  <nttp_to_type<LexState::Start>, charset_digits, nttp_to_type<LexState::Number>>,

		ENABLED  <nttp_to_type<LexState::Start>, charset<':'>, nttp_to_type<LexState::DelimiterColon>>,
		ENABLED  <nttp_to_type<LexState::Start>, charset<';'>, nttp_to_type<LexState::DelimiterSemi>>,
		ENABLED  <nttp_to_type<LexState::Start>, charset<','>, nttp_to_type<LexState::DelimiterComma>>,

		ENABLED  <nttp_to_type<LexState::Start>, charset<'('>, nttp_to_type<LexState::DelimiterLParen>>,
		ENABLED  <nttp_to_type<LexState::Start>, charset<')'>, nttp_to_type<LexState::DelimiterRParen>>,

		ENABLED  <nttp_to_type<LexState::Start>, charset<'{'>, nttp_to_type<LexState::DelimiterLCurly>>,
		ENABLED  <nttp_to_type<LexState::Start>, charset<'}'>, nttp_to_type<LexState::DelimiterRCurly>>,

		ENABLED  <nttp_to_type<LexState::Start>, charset<'['>, nttp_to_type<LexState::DelimiterLSquare>>,
		ENABLED  <nttp_to_type<LexState::Start>, charset<']'>, nttp_to_type<LexState::DelimiterRSquare>>,

		ENABLED  <nttp_to_type<LexState::Start>, charset<'<'>, nttp_to_type<LexState::DelimiterLAngle>>,
		ENABLED  <nttp_to_type<LexState::Start>, charset<'>'>, nttp_to_type<LexState::DelimiterRAngle>>,

		ENABLED  <nttp_to_type<LexState::Start>, charset<'\n'>, nttp_to_type<LexState::Newline>>,
		ENABLED  <nttp_to_type<LexState::Start>, charset_isdelimiter_pure, nttp_to_type<LexState::DelimiterOpaque>>,
		ENABLED  <nttp_to_type<LexState::Start>, charset_isoperator_pure, nttp_to_type<LexState::Operator>>,

		ENABLED  <nttp_to_type<LexState::Start>, charset_iswhitespace, nttp_to_type<LexState::Whitespace>>,
		ENABLED  <nttp_to_type<LexState::Start>, charset_ispreprocessor, nttp_to_type<LexState::Preprocessor>>,

		ENABLED  <nttp_to_type<LexState::Preprocessor>, charset_alpha, nttp_to_type<LexState::Preprocessor>>,
		ENABLED  <nttp_to_type<LexState::Identifier>, charset_alphanumeric, nttp_to_type<LexState::Identifier>>,

		ENABLED  <nttp_to_type<LexState::Number>, charset_digits, nttp_to_type<LexState::Number>>,
		ENABLED  <nttp_to_type<LexState::Operator>, charset_isoperator, nttp_to_type<LexState::Operator>>,

		ENABLED  <nttp_to_type<LexState::DelimiterColon>, charset<':'>, nttp_to_type<LexState::DelimiterColon>>,
		DISABLED <nttp_to_type<LexState::Whitespace>, charset_iswhitespace, nttp_to_type<LexState::Whitespace>>
	>
>;
#else
using LexingAutomaton__final = StaticDFA<
	StaticDfaTransitions<
		ENABLED  <nttp_to_type<LexState::Start>, nttp_to_type<'\n'>, nttp_to_type<LexState::Newline>>
	>
>;
#endif


using LexStateToTokenMapper__final = EnumMapper<
	EnumMapperConfiguration<
		ENABLED  <nttp_to_type<LexState::Identifier>,	        nttp_to_type<TokenKind::Identifier>>,
		ENABLED  <nttp_to_type<LexState::DelimiterOpaque>,      nttp_to_type<TokenKind::DelimiterOpaque>>,
		ENABLED  <nttp_to_type<LexState::Operator>,	        nttp_to_type<TokenKind::Operator>>,

		ENABLED  <nttp_to_type<LexState::DelimiterColon>,       nttp_to_type<TokenKind::DelimiterColon>>,
		ENABLED  <nttp_to_type<LexState::DelimiterSemi>,        nttp_to_type<TokenKind::DelimiterSemicolon>>,
		ENABLED  <nttp_to_type<LexState::DelimiterComma>,       nttp_to_type<TokenKind::DelimiterComma>>,

		ENABLED  <nttp_to_type<LexState::DelimiterLParen>,      nttp_to_type<TokenKind::DelimiterLParen>>,
		ENABLED  <nttp_to_type<LexState::DelimiterRParen>,      nttp_to_type<TokenKind::DelimiterRParen>>,

		ENABLED  <nttp_to_type<LexState::DelimiterLCurly>,      nttp_to_type<TokenKind::DelimiterLCurly>>,
		ENABLED  <nttp_to_type<LexState::DelimiterRCurly>,      nttp_to_type<TokenKind::DelimiterRCurly>>,

		ENABLED  <nttp_to_type<LexState::DelimiterLSquare>,     nttp_to_type<TokenKind::DelimiterLSquare>>,
		ENABLED  <nttp_to_type<LexState::DelimiterRSquare>,     nttp_to_type<TokenKind::DelimiterRSquare>>,

		ENABLED  <nttp_to_type<LexState::DelimiterLAngle>,      nttp_to_type<TokenKind::DelimiterLAngle>>,
		ENABLED  <nttp_to_type<LexState::DelimiterRAngle>,      nttp_to_type<TokenKind::DelimiterRAngle>>,

		ENABLED  <nttp_to_type<LexState::Preprocessor>,	        nttp_to_type<TokenKind::Preprocessor>>,
		ENABLED  <nttp_to_type<LexState::Newline>,	        nttp_to_type<TokenKind::Newline>>,
		ENABLED  <nttp_to_type<LexState::Number>,	        nttp_to_type<TokenKind::Number>>,

		CONDITIONAL <(0 == 0), nttp_to_type<LexState::Invalid>,	nttp_to_type<TokenKind::Unknown>>
	>
>;


using TokenKwrdCategorizer__final = TokenKeywordCategorizer<
	TokenKeywordCategorizerConfiguration<
		ENABLED  <AccessKeywordMatchingPolicy,    nttp_to_type<TokenKind::KeywordAccess>>,
		ENABLED  <AlignmentKeywordMatchingPolicy, nttp_to_type<TokenKind::KeywordAlignment>>,
		ENABLED  <ControlKeywordMatchingPolicy,   nttp_to_type<TokenKind::KeywordControl>>,
		ENABLED  <ModifierKeywordMatchingPolicy,  nttp_to_type<TokenKind::KeywordModifier>>,
		DISABLED <QualifierKeywordMatchingPolicy, nttp_to_type<TokenKind::KeywordQualifier>>,
		DISABLED <SpecifierKeywordMatchingPolicy, nttp_to_type<TokenKind::KeywordSpecifier>>,
		DISABLED <TypeKeywordMatchingPolicy,      nttp_to_type<TokenKind::KeywordType>>
	>
>;


using LexicalAnalyzer =
	Lexer<
		LexingAutomaton__final,
		LexStateToTokenMapper__final,
		TokenKwrdCategorizer__final
	>;


LexStateToTokenMapper__final mapper;

std::cout
	<< "mapping result: "
	<< static_cast<int>(mapper.find_target(LexState::Identifier))
	<< "\n";


std::string content = generate_lexer_stress_test();

LexicalAnalyzer lexer;

auto start = std::chrono::steady_clock::now();

std::vector<Token> tokens =
	lexer.tokenize<CharReader, PositionTracker>(content);

auto endA = std::chrono::steady_clock::now();
auto durationA =
	std::chrono::duration_cast<std::chrono::nanoseconds>(endA - start);

std::cout
	<< "[Tokenization] "
	<< durationA.count()
	<< " ns\n";


using test_configuration_static_dfa =
	generate_expanded_dfa_config_t<
		GenericConfigurationEntry<
			true,
			nttp_to_type<LexState::Start>,
			charset<'a', 'b', 'c'>,
			nttp_to_type<LexState::Newline>
		>
	>;


LexingAutomaton__final dfaf{};

std::cout
	<< "[dfaf -> before] result: "
	<< static_cast<int>(dfaf.get_current_state())
	<< "\n";

dfaf.step('d');

std::cout
	<< "[dfaf -> after] result: "
	<< static_cast<int>(dfaf.get_current_state())
	<< "\n";


std::cout
	<< "Generation result: "
	<< type_name<test_configuration_static_dfa>()
	<< "\n";
```
</details>




<details>
<summary>Expand to see the compile-time DFA generation pipeline and flat transition matrix</summary>

```cpp
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
	[[nodiscard]] constexpr const auto& at(
		std::size_t i,
		std::size_t j
	) const noexcept {

		if (i == -1 || i < 0) [[unlikely]] {
			static int invalid_state_fallback = -1;
			return invalid_state_fallback;
		}

		return m_[i * C + j];
	}

	[[nodiscard]] constexpr auto& at(
		std::size_t i,
		std::size_t j
	) noexcept {

		if (i == -1 || i < 0) [[unlikely]] {
			static int invalid_state_fallback = -1;
			return invalid_state_fallback;
		}

		return m_[i * C + j];
	}

private:
	std::array<int, R * C> m_;
};


/********************************************************************************************/


template<std::size_t RS, std::size_t RC>
struct FlatMatrixDFA final {
protected:
	using integer_type = int;

	static constexpr integer_type default_state_start = 0;
	static constexpr integer_type default_invalid_state = -1;

	static constexpr std::array<
		integer_type,
		RS * RC
	> default_matrix_table = []() {

		std::array<integer_type, RS * RC> temp;
		temp.fill(-1);

		return temp;
	}();

public:
	explicit consteval FlatMatrixDFA(
		std::array<integer_type, RS * RC> table =
			default_matrix_table,

		integer_type start_state =
			default_state_start,

		integer_type invalid_state =
			default_invalid_state
	)
		: matrix_{ table }
		, start_state_(start_state)
		, invalid_state_(invalid_state)
	{
	}

public:
	[[nodiscard]]
	constexpr bool step(int predicate) {

		if (predicate < 0 || predicate >= RC) [[unlikely]] {
			return false;
		}

		previous_state_ = current_state_;
		current_state_ =
			matrix_.at(current_state_, predicate);

		return current_state_ != default_invalid_state;
	}

	[[nodiscard]]
	constexpr int get_current() const noexcept {
		return current_state_;
	}

	[[nodiscard]]
	constexpr int get_previous() const noexcept {
		return previous_state_;
	}

public:
	void reset() noexcept {
		current_state_ = start_state_;
		previous_state_ = start_state_;
	}

private:
	StaticMatrix<RS, RC> matrix_;

	integer_type start_state_;
	integer_type invalid_state_;

	integer_type current_state_ = start_state_;
	integer_type previous_state_ = start_state_;
};


/********************************************************************************************/


template<typename entry_tuple>
struct StaticDFAConfigurationSchema {
	using first_t =
		std::decay_t<std::tuple_element_t<0, entry_tuple>>;

	using second_t =
		std::decay_t<std::tuple_element_t<1, entry_tuple>>;

	using third_t =
		std::decay_t<std::tuple_element_t<2, entry_tuple>>;

	static constexpr bool valid =
		requires {
			requires std::tuple_size_v<entry_tuple> == 3;

			requires
				!std::same_as<
					decltype(first_t::value),
					decltype(second_t::value)
				>;

			requires
				std::same_as<
					decltype(first_t::value),
					decltype(third_t::value)
				>;
		};
};


template<typename entry_tuple>
struct StaticDFAConfigurationModel final {
	static constexpr auto source =
		std::tuple_element_t<0, entry_tuple>::value;

	static constexpr auto predicate =
		std::tuple_element_t<1, entry_tuple>::value;

	static constexpr auto target =
		std::tuple_element_t<2, entry_tuple>::value;
};


template<typename... Entries>
using StaticDfaTransitions = ConfigurationSystem<
	StaticDFAConfigurationSchema,
	StaticDFAConfigurationModel,
	Entries...
>;


/********************************************************************************************/


template <typename T>
constexpr bool is_static_dfa_config_v = false;

template <typename... Entries>
constexpr bool is_static_dfa_config_v<
	StaticDfaTransitions<Entries...>
> = true;

template <typename T>
concept is_static_dfa_configuration =
	is_static_dfa_config_v<T>;


/********************************************************************************************/


template<is_static_dfa_configuration Configuration>
struct StaticDFA final {
protected:
	using integer_type = int;

	using configuration_tuple_t =
		typename Configuration::type;

	using first_entry_type =
		std::tuple_element_t<
			0,
			configuration_tuple_t
		>;

	using row_type =
		std::decay_t<
			decltype(first_entry_type::source)
		>;

	using column_type =
		std::decay_t<
			decltype(first_entry_type::predicate)
		>;

protected:
	static constexpr std::size_t row_maximum_size =
		[]<typename... Entries>(
			std::tuple<Entries...>*
		) {

			return std::max(
				std::max({
					static_cast<integer_type>(
						Entries::source
					)...
				}),

				std::max({
					static_cast<integer_type>(
						Entries::target
					)...
				})
			) + 1;

		}(static_cast<configuration_tuple_t*>(nullptr));


	static constexpr std::size_t column_maximum_size =
		[]<typename... Entries>(
			std::tuple<Entries...>*
		) {

			return std::max({
				static_cast<integer_type>(
					Entries::predicate
				)...
			}) + 1;

		}(static_cast<configuration_tuple_t*>(nullptr));


public:
	[[nodiscard]]
	constexpr bool step(column_type predicate) {
		return dfa_.step(
			static_cast<integer_type>(predicate)
		);
	}


	[[nodiscard]]
	constexpr row_type get_current_state()
		const noexcept(noexcept(dfa_.get_current()))
	{
		return static_cast<row_type>(
			dfa_.get_current()
		);
	}


	[[nodiscard]]
	constexpr row_type get_previous_state()
		const noexcept(noexcept(dfa_.get_previous()))
	{
		return static_cast<row_type>(
			dfa_.get_previous()
		);
	}


public:
	void reset() noexcept(
		noexcept(dfa_.reset())
	) {
		dfa_.reset();
	}


private:
	static consteval std::array<
		integer_type,
		row_maximum_size * column_maximum_size
	> generateDfaTable() {

		auto temp = []() {

			std::array<
				integer_type,
				row_maximum_size * column_maximum_size
			> arr;

			arr.fill(-1);

			return arr;

		}();


		[]<typename... Entries>(
			std::tuple<Entries...>*,
			auto& table
		) {

			(
				(
					table[
						static_cast<integer_type>(
							Entries::source
						) * column_maximum_size
						+
						static_cast<integer_type>(
							Entries::predicate
						)
					]
					=
					static_cast<integer_type>(
						Entries::target
					)
				),
				...
			);

		}(
			static_cast<configuration_tuple_t*>(nullptr),
			temp
		);

		return temp;
	}


private:
	FlatMatrixDFA<
		row_maximum_size,
		column_maximum_size
	> dfa_{
		generateDfaTable()
	};
};


/********************************************************************************************/


template<char... c>
struct charset {
};


/********************************************************************************************/


template<typename... Entries>
struct generate_expanded_dfa_config {

	using entry_tuple_t =
		std::tuple<
			make_tuple_from_config_entry_t<Entries>...
		>;


	static constexpr bool respects_all_conditions =
		[]<typename... entry_tuples>(
			std::tuple<entry_tuples...>*
		) {

			return ((

				std::tuple_size_v<entry_tuples> == 4

				&&

				std::is_same_v<
					decltype(
						std::tuple_element_t<
							1,
							entry_tuples
						>::value
					),

					decltype(
						std::tuple_element_t<
							3,
							entry_tuples
						>::value
					)
				)

			) && ...);

		}(static_cast<entry_tuple_t*>(nullptr));


	static_assert(
		respects_all_conditions,
		"mettre un message ici"
	);


	using generated_tuple_t =
		decltype([]<typename Tuple>(
			std::type_identity<Tuple>
		) {

			return []<typename... Ts>(
				std::type_identity<std::tuple<Ts...>>
			) {

				return std::tuple_cat(

					[]<typename SingleEntry>() {

						using CurrentCharset =
							std::tuple_element_t<
								2,
								SingleEntry
							>;

						return []<char... Characters>(
							charset<Characters...>*
						) {

							return std::tuple<
								GenericConfigurationEntry<
									std::tuple_element_t<
										0,
										SingleEntry
									>::value,

									std::tuple_element_t<
										1,
										SingleEntry
									>,

									nttp_to_type<Characters>,

									std::tuple_element_t<
										3,
										SingleEntry
									>
								>...
							>{};

						}(
							static_cast<
								CurrentCharset*
							>(nullptr)
						);

					}.template operator()<Ts>()...

				);

			}(
				std::type_identity<Tuple>{}
			);

		}(
			std::type_identity<entry_tuple_t>{}
		));


	using type =
		decltype([]<typename... entries>(
			std::tuple<entries...>*
		) {

			return StaticDfaTransitions<
				entries...
			>{};

		}(
			static_cast<
				generated_tuple_t*
			>(nullptr)
		));
};


template<typename... Entries>
using generate_expanded_dfa_config_t =
	typename generate_expanded_dfa_config<
		Entries...
	>::type;
```
</details>




<details>
<summary>Expand to see the compile-time bidirectional enum mapping system</summary>


```cpp
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
using EnumMapperConfiguration = ConfigurationSystem<
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
		return[&]<typename... Ts>(std::type_identity<std::tuple<Ts...>>) {
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
```
</details>


<details>
<summary>Expand to see the compile-time keyword-to-token categorization system</summary>


```cpp
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
	using TokenKeywordCategorizerConfiguration = ConfigurationSystem<
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
```
</details>


<details>
<summary>Expand to see the compile-time character sets and C++ keyword classification</summary>

```cpp
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
```
</details>



