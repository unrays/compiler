//#pragma once
//#include "../core/Concepts/is_split_string_strategy.hpp"
//#include "../core/macros.hpp"
import <string_view>;
import <string>;
import <vector>;

import split_string_strategy_base_module;
import string_splitting_context_module;

export module string_splitter_module;

//#include "split_string_strategy_base.cppm"

// e26-unrays/Projet/include/Core/concepts/is_split_string_strategy.hpp
/*************************************************************/

export template<typename T>
concept ISplitStringStrategy = std::derived_from<T, SplitStringStrategyBase<T>>;

/*************************************************************/

export template<ISplitStringStrategy Strategy>
struct StringSplitter {
    [[nodiscard]]
    const auto split(const std::string_view& input) const {
        std::vector<std::string> accumulator;
        auto context = StringSplittingContext{ input, accumulator };

        Strategy{}.apply(context); // illegal sérieux, stateless

        return context.accumulator;
    }
};