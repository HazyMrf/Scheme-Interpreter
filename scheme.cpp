#include <sstream>
#include "scheme.h"

// #define VALIDATE_LAMBDA_VARIABLE_LIST(variables)

std::string Serialize(std::shared_ptr<Object> cell) {
    if (cell == nullptr) {  // the empty list
        return "()";
    }
    return cell->Stringify();
}

void ArgsToVector(std::vector<std::shared_ptr<Object>>& vec, std::shared_ptr<Object> cell,
                  const std::string& name, std::shared_ptr<Scope>& scope) {
    if (cell == nullptr) {
        return;
    }
    if (Is<Number>(cell) || Is<Boolean>(cell)) {
        vec.push_back(cell);
    } else if (Is<Symbol>(cell)) {
        if (scope->AllContains(As<Symbol>(cell)->GetName())) {
            vec.push_back(scope->AllResolveSymbol(As<Symbol>(cell)->GetName()));
        }
    } else {
        auto head = As<Cell>(cell)->GetFirst();
        auto tail = As<Cell>(cell)->GetSecond();
        if (Is<Symbol>(head)) {
            if (As<Symbol>(head)->GetName() == "lambda") {
                if (!Is<Cell>(tail)) {
                    throw SyntaxError("Lambda arguments passed without brackets");
                }
                auto variables = As<Cell>(tail)->GetFirst();
                auto evals = As<Cell>(tail)->GetSecond();

                std::vector<std::string> arguments;
                while (variables != nullptr) {
                    if (!Is<Cell>(variables)) {
                        throw SyntaxError("invalid lambda variables");
                    }
                    if (!Is<Symbol>(As<Cell>(variables)->GetFirst())) {
                        throw SyntaxError("bad lambda variables");
                    }
                    arguments.push_back(As<Symbol>(As<Cell>(variables)->GetFirst())->GetName());
                    variables = As<Cell>(variables)->GetSecond();
                }

                std::vector<std::shared_ptr<Object>> evals_objects;
                if (evals == nullptr) {
                    throw SyntaxError("No return value for your lambda function");
                }
                while (evals != nullptr) {
                    if (!Is<Cell>(evals)) {
                        throw SyntaxError("invalid lambda variables in return");
                    }
                    evals_objects.push_back(As<Cell>(evals)->GetFirst());
                    evals = As<Cell>(evals)->GetSecond();
                }

                auto lambda_creator =
                        std::make_shared<LambdaCreator>(scope, arguments, evals_objects);
                vec.push_back(lambda_creator);
                return;
            }
            if (name == "and" || name == "or") {
                try {  // case of boolean short-circuit functions
                    head->Eval(scope);
                    if (As<Symbol>(head)->GetName() == "quote") {
                        vec.push_back(cell);
                        return;
                    }
                } catch (const NameError&) {  // if the name of func is invalid, we add it's body
                    vec.push_back(cell);
                    return;
                }
            }
            if (!scope->AllContains(As<Symbol>(head)->GetName())) {
                throw NameError(As<Symbol>(head)->GetName());
            }
            if (!Is<Function>(scope->AllResolveSymbol(As<Symbol>(head)->GetName())) &&
                !Is<LambdaCreator>(scope->AllResolveSymbol(As<Symbol>(head)->GetName()))) {
                vec.push_back(scope->AllResolveSymbol(As<Symbol>(head)->GetName()));
                ArgsToVector(vec, tail, name, scope);
                return;
            }
            auto obj = Evaluate(cell, scope);
            vec.push_back(obj);
        } else {
            if (head == nullptr && tail == nullptr) {
                throw RuntimeError("No adding '()' to argument list");
            }
            ArgsToVector(vec, head, name, scope);
            if (name == "if") {
                if (!Is<Cell>(tail)) {
                    throw SyntaxError("Invalid number of args to function 'if");
                }
                vec.push_back(As<Cell>(tail)->GetFirst());
                tail = As<Cell>(tail)->GetSecond();
                if (Is<Cell>(tail)) {
                    vec.push_back(As<Cell>(tail)->GetFirst());
                    if (As<Cell>(tail)->GetSecond() != nullptr) {
                        throw SyntaxError("Too many arguments passed to function 'if'");
                    }
                }
                return;
            }
            ArgsToVector(vec, tail, name, scope);
        }
    }
}

std::shared_ptr<Object> Evaluate(std::shared_ptr<Object> cell, std::shared_ptr<Scope>& scope) {
    if (cell == nullptr) {
        throw RuntimeError("You've done something strange...");
    }
    if (Is<Number>(cell) || Is<Boolean>(cell)) {
        return cell->Eval(scope);
    }
    if (Is<Symbol>(cell)) {
        if (scope->AllContains(As<Symbol>(cell)->GetName())) {
            return scope->AllResolveSymbol(As<Symbol>(cell)->GetName());
        }
        throw NameError("Passed parameter is not a number, boolean or function");
    }
    auto head = As<Cell>(cell)->GetFirst();
    auto tail = As<Cell>(cell)->GetSecond();
    if (Is<Symbol>(head)) {
        if (As<Symbol>(head)->GetName() == "lambda") {
            if (!Is<Cell>(tail)) {
                throw SyntaxError("Lambda arguments passed without brackets");
            }
            auto variables = As<Cell>(tail)->GetFirst();
            auto evals = As<Cell>(tail)->GetSecond();
            if (evals == nullptr) {
                throw SyntaxError("No return value for your lambda function");
            }

            std::vector<std::string> arguments;
            while (variables != nullptr) {
                if (!Is<Cell>(variables)) {
                    throw SyntaxError("invalid lambda variables");
                }
                if (!Is<Symbol>(As<Cell>(variables)->GetFirst())) {
                    throw SyntaxError("bad lambda variables");
                }
                arguments.push_back(As<Symbol>(As<Cell>(variables)->GetFirst())->GetName());
                variables = As<Cell>(variables)->GetSecond();
            }

            std::vector<std::shared_ptr<Object>> evals_objects;
            if (evals == nullptr) {
                throw SyntaxError("No return value for your lambda function");
            }
            while (evals != nullptr) {
                if (!Is<Cell>(evals)) {
                    throw SyntaxError("invalid lambda variables in return");
                }
                evals_objects.push_back(As<Cell>(evals)->GetFirst());
                evals = As<Cell>(evals)->GetSecond();
            }
            auto lambda_creator = std::make_shared<LambdaCreator>(scope, arguments, evals_objects);
            return lambda_creator;
        }
        if (As<Symbol>(head)->GetName() == "quote") {
            if (tail == nullptr) {  // corner case of '()
                return tail;
            }
            return As<Cell>(tail)->GetFirst();  // quote leaves object and does not evaluate it
        }
        std::vector<std::shared_ptr<Object>> args;
        if (As<Symbol>(head)->GetName() == "define" || As<Symbol>(head)->GetName() == "set!") {
            if (!Is<Cell>(tail)) {
                throw SyntaxError("define and set! functions have 2 arguments: variable and value");
            }
            if (!Is<Symbol>(As<Cell>(tail)->GetFirst())) {
                // the case of lambda being defined using syntax (define (name ...args) body)
                if (!Is<Cell>(As<Cell>(tail)->GetFirst())) {
                    throw SyntaxError("Invalid lambda");
                }

                auto lambda_name_variables = As<Cell>(As<Cell>(tail)->GetFirst());
                auto name = lambda_name_variables->GetFirst();
                auto variables = lambda_name_variables->GetSecond();

                if (!Is<Symbol>(name)) {
                    throw SyntaxError("Invalid lambda name in lambda-sugar");
                }

                auto new_cell = std::make_shared<Cell>(std::make_shared<Symbol>("define"), nullptr);
                new_cell->GetSecond() = std::make_shared<Cell>(name, nullptr);
                As<Cell>(new_cell->GetSecond())->GetSecond() = std::make_shared<Cell>(
                        std::make_shared<Cell>(
                                std::make_shared<Symbol>("lambda"),
                                std::make_shared<Cell>(variables, As<Cell>(tail)->GetSecond())),
                        nullptr);
                return Evaluate(new_cell, scope);
            }
            args.push_back(As<Cell>(tail)->GetFirst());
            tail = As<Cell>(tail)->GetSecond();
            if (!Is<Cell>(tail)) {
                throw SyntaxError("invalid arguments were passed");
            }
        }
        ArgsToVector(args, tail, As<Symbol>(head)->GetName(), scope);
        if (Is<LambdaCreator>(head->Eval(scope))) {
            auto lambda = As<LambdaCreator>(head->Eval(scope))->CreateLambda();
            return lambda->EvalLambda(args);
        }
        return As<Function>(head->Eval(scope))->Apply(scope, args);
    } else if (Is<Cell>(head) && Is<Symbol>(As<Cell>(head)->GetFirst()) &&
               As<Symbol>(As<Cell>(head)->GetFirst())->GetName() == "lambda") {

        auto lambda_head = As<Cell>(head)->GetFirst();
        auto lambda_tail = As<Cell>(head)->GetSecond();

        if (!Is<Cell>(lambda_tail)) {
            throw SyntaxError("Lambda arguments passed without brackets");
        }
        auto variables = As<Cell>(lambda_tail)->GetFirst();
        auto evals = As<Cell>(lambda_tail)->GetSecond();

        std::vector<std::string> arguments;
        while (variables != nullptr) {
            if (!Is<Cell>(variables)) {
                throw SyntaxError("invalid lambda variables");
            }
            if (!Is<Symbol>(As<Cell>(variables)->GetFirst())) {
                throw SyntaxError("bad lambda variables");
            }
            arguments.push_back(As<Symbol>(As<Cell>(variables)->GetFirst())->GetName());
            variables = As<Cell>(variables)->GetSecond();
        }

        std::vector<std::shared_ptr<Object>> evals_objects;
        if (evals == nullptr) {
            throw SyntaxError("No return value for your lambda function");
        }
        while (evals != nullptr) {
            if (!Is<Cell>(evals)) {
                throw SyntaxError("invalid lambda variables in return");
            }
            evals_objects.push_back(As<Cell>(evals)->GetFirst());
            evals = As<Cell>(evals)->GetSecond();
        }

        auto lambda = std::make_shared<Lambda>(scope, arguments, evals_objects);
        std::vector<std::shared_ptr<Object>> args;
        ArgsToVector(args, tail, "lambda", scope);
        return lambda->EvalLambda(args);
    } else if (Is<Cell>(head)) {
        if (!Is<Symbol>(As<Cell>(head)->GetFirst())) {
            throw RuntimeError("first goes func name");
        }
        auto lambda_creator = Evaluate(head, scope);  // case of return-lambda invoker
        if (Is<LambdaCreator>(lambda_creator)) {
            std::vector<std::shared_ptr<Object>> e;
            ArgsToVector(e, tail, "lambda", scope);
            return As<LambdaCreator>(lambda_creator)->CreateLambda()->EvalLambda(e);
        } else {
            throw RuntimeError("some very strange error");
        }
    } else {
        throw RuntimeError("You've done something strange...");
    }
}

std::string Interpreter::Run(const std::string& input) {
    std::stringstream ss{input.data()};
    Tokenizer tokenizer(&ss);

    auto input_ast = Read(&tokenizer);

    auto output_ast = Evaluate(input_ast, global_scope_);

    std::string result = Serialize(output_ast);

    // add cleanup (tidy)

    return result;
}
