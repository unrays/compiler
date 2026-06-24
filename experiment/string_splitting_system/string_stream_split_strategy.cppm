//#pragma once
import <utility>;
import <sstream> ;
import <iostream>;

//#include "../core/base/split_string_strategy_base.hpp"
//#include "string_splitting_context.hpp"

//import split_string_strategy_base_module;
//#include "split_string_strategy_base.cppm"

import split_string_strategy_base_module;

export module string_split_strategy_module;

//#include "string_splitting_context.cppm"

export struct StringStreamSplitStrategy :
    public SplitStringStrategyBase<StringStreamSplitStrategy> {
    friend SplitStringStrategyBase<StringStreamSplitStrategy>;
private:
    void apply_impl(StringSplittingContext& context) const {
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

/*

std::stringstream ss(std::string{ context.input });

        std::string current;
        while (ss >> current)
            context.accumulator.push_back(current);
            */

/*
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
*/