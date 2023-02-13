#pragma once

#include <string>
#include <vector>
#include <tokenizer.h>
#include <parser.h>

const std::unordered_map<std::string, std::shared_ptr<Object>> kDefaultFuncs = {
        // Quote
        {"quote", std::make_shared<Quote>()},

        // Symbol
        {"symbol?", std::make_shared<IsSymbol>()},

        // Boolean
        {"boolean?", std::make_shared<IsBoolean>()},
        {"not", std::make_shared<BooleanNot>()},
        {"and", std::make_shared<BooleanAnd>()},
        {"or", std::make_shared<BooleanOr>()},

        // List
        {"pair?", std::make_shared<IsPair>()},
        {"null?", std::make_shared<IsNull>()},
        {"list?", std::make_shared<IsList>()},
        {"cons", std::make_shared<Cons>()},
        {"car", std::make_shared<Car>()},
        {"cdr", std::make_shared<Cdr>()},
        {"list", std::make_shared<CreateList>()},
        {"list-ref", std::make_shared<ListRef>()},
        {"list-tail", std::make_shared<ListTail>()},

        // Integer
        {"number?", std::make_shared<IsNumber>()},
        {"=", std::make_shared<Equal>()},
        {">", std::make_shared<Greater>()},
        {"<", std::make_shared<Less>()},
        {">=", std::make_shared<GreaterOrEqual>()},
        {"<=", std::make_shared<LessOrEqual>()},
        {"+", std::make_shared<Add>()},
        {"-", std::make_shared<Sub>()},
        {"*", std::make_shared<Multiply>()},
        {"/", std::make_shared<Divide>()},
        {"max", std::make_shared<Max>()},
        {"min", std::make_shared<Min>()},
        {"abs", std::make_shared<Abs>()},

        // Advanced
        {"define", std::make_shared<Define>()},
        {"set!", std::make_shared<Set>()},
        {"if", std::make_shared<If>()},
        {"set-car!", std::make_shared<SetCar>()},
        {"set-cdr!", std::make_shared<SetCdr>()},
        //    {"lambda", std::make_shared<Lambda>()}

        // If not found, then NameError is to be thrown
};

class Interpreter {
public:
    Interpreter() : global_scope_(std::make_shared<Scope>(kDefaultFuncs)) {
    }

    std::string Run(const std::string& input);

private:
    std::shared_ptr<Scope> global_scope_;
};
