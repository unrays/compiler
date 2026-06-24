#pragma once
import <utility>;

import token_module;

export module lexing_state_type_module;


export enum struct LexingStateType {
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