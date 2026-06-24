export module split_string_strategy_base_module;

//#include "string_splitting_context.cppm"
import string_splitting_context_module;

export template<typename Derived>
struct SplitStringStrategyBase {
    const auto apply(StringSplittingContext& context) {
        static_cast<Derived*>(this)->apply_impl(context);
    }
};