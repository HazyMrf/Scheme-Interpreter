#include <tokenizer.h>
#include <ctype.h>
#include "error.h"

bool SymbolToken::operator==(const SymbolToken &other) const {
    return name == other.name;
}

bool QuoteToken::operator==(const QuoteToken &) const {
    return true;
}

bool DotToken::operator==(const DotToken &) const {
    return true;
}

bool ConstantToken::operator==(const ConstantToken &other) const {
    return value == other.value;
}

bool BooleanToken::operator==(const BooleanToken &other) const {
    return (b == other.b);
}

Tokenizer::Tokenizer(std::istream *in) : in_(in) {
    Next();
}

bool Tokenizer::IsEnd() {
    return is_end_;
}

void Tokenizer::Next() {
    while (in_->peek() == ' ' || in_->peek() == '\n' || in_->peek() == '\t' ||
           in_->peek() == '\v' || in_->peek() == '\r' || in_->peek() == '\f') {
        in_->get();
    }
    if (in_->peek() == EOF) {
        is_end_ = true;
        return;
    }

    // start getting the token
    if (std::isdigit(in_->peek())) {
        ReadConstantToken("");
    } else if (in_->peek() == '(') {
        in_->get();
        token_ = BracketToken::OPEN;
    } else if (in_->peek() == ')') {
        in_->get();
        token_ = BracketToken::CLOSE;
    } else if (in_->peek() == '.') {
        in_->get();
        token_ = DotToken{};
    } else if (in_->peek() == '\'') {
        in_->get();
        token_ = QuoteToken{};
    } else {
        if (kSymbolTokenStartChars.find(static_cast<char>(in_->peek())) != std::string::npos) {
            ReadSymbolOrBooleanToken("");
        } else if (in_->peek() == '+') {
            in_->get();
            if (std::isdigit(in_->peek())) {
                ReadConstantToken("+");
            } else {
                token_ = SymbolToken{"+"};
            }
        } else if (in_->peek() == '-') {
            in_->get();
            if (std::isdigit(in_->peek())) {
                ReadConstantToken("-");
            } else {
                token_ = SymbolToken{"-"};
            }
        } else {
            throw SyntaxError("Invalid Arguments");
        }
    }
}

Token Tokenizer::GetToken() {
    return token_;
}

void Tokenizer::ReadConstantToken(std::string signed_number) {
    while (std::isdigit(in_->peek())) {
        signed_number += static_cast<char>(in_->get());
    }
    token_ = ConstantToken{std::stoi(signed_number)};
}

void Tokenizer::ReadSymbolOrBooleanToken(std::string string_of_symbols) {
    while (kSymbolTokenChars.find(static_cast<char>(in_->peek())) != std::string::npos) {
        string_of_symbols += static_cast<char>(in_->get());
    }
    if (string_of_symbols == "#t") {
        token_ = BooleanToken{true};
    } else if (string_of_symbols == "#f") {
        token_ = BooleanToken{false};
    } else {
        token_ = SymbolToken{string_of_symbols};
    }
}
