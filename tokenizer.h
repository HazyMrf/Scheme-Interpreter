#pragma once

#include <variant>
#include <optional>
#include <istream>

const std::string kSymbolTokenStartChars =
        "[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ<=>*/#]";
const std::string kSymbolTokenChars =
        "[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ<=>*/#0123456789?!-]";

struct SymbolToken {
    std::string name;

    bool operator==(const SymbolToken& other) const;
};

struct QuoteToken {
    bool operator==(const QuoteToken&) const;
};

struct DotToken {
    bool operator==(const DotToken&) const;
};

enum class BracketToken { OPEN, CLOSE };

struct ConstantToken {
    int value;

    bool operator==(const ConstantToken& other) const;
};

struct BooleanToken {
    bool b;

    bool operator==(const BooleanToken& other) const;
};

using Token =
        std::variant<ConstantToken, BracketToken, SymbolToken, QuoteToken, DotToken, BooleanToken>;

class Tokenizer {
public:
    Tokenizer(std::istream* in);

    bool IsEnd();

    void Next();

    Token GetToken();

private:
    void ReadConstantToken(std::string sign);

    void ReadSymbolOrBooleanToken(std::string string_of_symbols);

private:
    std::istream* in_;
    Token token_;
    bool is_end_ = false;
};