//#pragma once
import <string_view>;
import <string>;
import <vector>;

export module string_splitting_context_module;

export struct StringSplittingContext {
    std::string_view input;
    std::vector<std::string>& accumulator;

    explicit StringSplittingContext(std::string_view input_,
        std::vector<std::string>& accumulator_)
        : input(input_)
        , accumulator(accumulator_) {
    }
};