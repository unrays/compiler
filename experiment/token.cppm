import <string>;

export module token_module;

export enum struct TokenType {
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

export struct Token {
    TokenType type;
    std::string lexeme;
};