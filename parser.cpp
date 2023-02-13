#include <parser.h>
#include <error.h>

std::shared_ptr<Object> ReadInner(Tokenizer* tokenizer) {
    if (tokenizer->IsEnd()) {
        throw SyntaxError("Empty argument list");
    }
    auto token = tokenizer->GetToken();
    tokenizer->Next();
    // we use std::hold_alternative instead of using std::get_if<Type>(&value)
    if (std::holds_alternative<ConstantToken>(token)) {
        return std::make_shared<Number>(std::get_if<ConstantToken>(&token)->value);
    } else if (std::holds_alternative<SymbolToken>(token)) {
        return std::make_shared<Symbol>(std::get_if<SymbolToken>(&token)->name);
    } else if (std::holds_alternative<BooleanToken>(token)) {
        return std::make_shared<Boolean>(std::get_if<BooleanToken>(&token)->b);
    } else if (std::holds_alternative<BracketToken>(token) &&
               std::get<BracketToken>(token) == BracketToken::OPEN) {
        auto sh_ptr = ReadList(tokenizer);
        return sh_ptr;
    } else if (std::holds_alternative<DotToken>(token)) {
        return std::make_shared<Symbol>(".");  // handling the case of dot by creating Symbol{"."}
    } else if (std::holds_alternative<QuoteToken>(token)) {
        auto head = std::make_shared<Symbol>("quote");
        if (tokenizer->IsEnd()) {
            throw SyntaxError("Arguments must be passed to function quote");
        }
        auto tail = ReadInner(tokenizer);
        if (tail == nullptr) {
            return std::make_shared<Cell>(head, tail);
        }
        auto tail_null = std::make_shared<Cell>(tail, nullptr);
        return std::make_shared<Cell>(head, tail_null);
    } else {
        throw SyntaxError("Invalid Arguments while parsing");
    }
}

std::shared_ptr<Object> ReadList(Tokenizer* tokenizer) {
    std::shared_ptr<Object> result_cell = nullptr;
    std::shared_ptr<Cell> cur_cell{};
    while (!tokenizer->IsEnd()) {
        if (std::holds_alternative<BracketToken>(tokenizer->GetToken()) &&
            std::get<BracketToken>(tokenizer->GetToken()) == BracketToken::CLOSE) {
            tokenizer->Next();
            return result_cell;
        }
        auto sh_ptr = ReadInner(tokenizer);
        if (Is<Symbol>(sh_ptr) && As<Symbol>(sh_ptr)->GetName() == ".") {  // dot
            if (result_cell == nullptr || tokenizer->IsEnd()) {
                throw SyntaxError("Using dot before passing the first argument to pair");
            } else {
                cur_cell->GetSecond() = ReadInner(tokenizer);
                if (Is<Symbol>(cur_cell->GetSecond())) {
                    throw SyntaxError("Invalid syntax");
                }
                if (tokenizer->IsEnd() ||
                    !(std::holds_alternative<BracketToken>(tokenizer->GetToken()) &&
                      std::get<BracketToken>(tokenizer->GetToken()) == BracketToken::CLOSE)) {
                    throw SyntaxError("No closing bracket for pair");
                }
                tokenizer->Next();
                return result_cell;
            }
        }
        if (result_cell == nullptr) {
            cur_cell = std::make_shared<Cell>(sh_ptr, nullptr);
            result_cell = cur_cell;
        } else {
            auto tail_cell = std::make_shared<Cell>(sh_ptr, nullptr);
            cur_cell->GetSecond() = tail_cell;
            cur_cell = tail_cell;
        }
    }
    throw SyntaxError("No closing bracket for list");
}

std::shared_ptr<Object> Read(Tokenizer* tokenizer) {
    auto obj = ReadInner(tokenizer);
    if (!tokenizer->IsEnd()) {
        throw SyntaxError("Too many arguments passed");
    }
    return obj;
}