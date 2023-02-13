#pragma once

#include <memory>
#include <vector>
#include <error.h>
#include <unordered_map>
#include <cassert>

#define ASSERT_ALL_NUMBERS(objects)                                    \
    for (size_t i = 0; i < objects.size(); ++i) {                      \
        if (!Is<Number>(objects[i])) {                                 \
            throw RuntimeError("Argument is expected to be a number"); \
        }                                                              \
    }

class Object;

// Scope -- special object
///////////////////////////////////////////////////////////////////////////////

class Scope {
public:
    Scope() : parent_scope_(nullptr), scope_objects_({}) {
    }

    Scope(std::shared_ptr<Scope>& parent) : parent_scope_(parent), scope_objects_({}) {
    }

    Scope(const std::unordered_map<std::string, std::shared_ptr<Object>> default_variables)
            : parent_scope_(nullptr), scope_objects_(default_variables) {
    }

    Scope(std::shared_ptr<Scope> parent,
          const std::unordered_map<std::string, std::shared_ptr<Object>>& default_variables)
            : parent_scope_(parent), scope_objects_(default_variables) {
    }

    bool Contains(const std::string& name) {
        return scope_objects_.contains(name);
    }

    bool AllContains(const std::string& name) {
        if (scope_objects_.contains(name)) {
            return true;
        }
        if (parent_scope_ == nullptr) {
            return false;
        }
        return parent_scope_->AllContains(name);
    }

    std::shared_ptr<Object> ResolveSymbol(const std::string& name) {
        return scope_objects_[name];
    }

    std::shared_ptr<Object> AllResolveSymbol(const std::string& name) {
        if (scope_objects_.contains(name)) {
            return scope_objects_[name];
        }
        return parent_scope_->AllResolveSymbol(name);
    }

    std::shared_ptr<Scope>& GetParentScope() {
        return parent_scope_;
    }

    void DefineSymbol(const std::string& name, std::shared_ptr<Object>& value) {
        scope_objects_[name] = value;
    }

    void SetDefineSymbol(const std::string& name, std::shared_ptr<Object>& value) {
        if (scope_objects_.contains(name)) {
            scope_objects_[name] = value;
        } else {
            if (parent_scope_ == nullptr) {
                throw SyntaxError("bad SetDefineSymbol");
            }
            parent_scope_->SetDefineSymbol(name, value);
        }
    }

private:
    std::shared_ptr<Scope> parent_scope_;  // parent scope
    std::unordered_map<std::string, std::shared_ptr<Object>> scope_objects_;
};

class Object : public std::enable_shared_from_this<Object> {
public:
    virtual ~Object() = default;
    virtual std::shared_ptr<Object> Eval(std::shared_ptr<Scope>& scope) = 0;
    virtual std::string Stringify() = 0;
};

std::shared_ptr<Object> ChooseFunction(const std::string& func_name);

///////////////////////////////////////////////////////////////////////////////

// Runtime type checking and conversion.
// This can be helpful: https://en.cppreference.com/w/cpp/memory/shared_ptr/pointer_cast

template <class DerivedFromObject>
std::shared_ptr<DerivedFromObject> As(const std::shared_ptr<Object>& obj) {
    return std::dynamic_pointer_cast<DerivedFromObject>(obj);
}

template <class DerivedFromObject>
bool Is(const std::shared_ptr<Object>& obj) {
    if (std::dynamic_pointer_cast<DerivedFromObject>(obj) == nullptr) {
        return false;
    } else {
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////

class Number : public Object {
public:
    Number(int64_t value) : value_(value) {
    }
    int64_t GetValue() const {
        return value_;
    }

    std::shared_ptr<Object> Eval(std::shared_ptr<Scope>&) override {
        return shared_from_this();
    }

    std::string Stringify() override {
        return std::to_string(value_);
    }

private:
    int64_t value_;
};

class Symbol : public Object {
public:
    Symbol(const std::string& name) : name_(name) {
    }
    const std::string& GetName() const {
        return name_;
    }

    // DO SCOPE
    std::shared_ptr<Object> Eval(std::shared_ptr<Scope>& scope) override {
        if (scope->Contains(name_)) {
            return scope->ResolveSymbol(name_);
        }
        if (scope->GetParentScope() == nullptr) {
            throw NameError(name_);
        }
        return this->Eval(scope->GetParentScope());
        //        return ChooseFunction(name_);
    }

    std::string Stringify() override {
        return name_;
    }

private:
    std::string name_;
};

class Boolean : public Object {
public:
    Boolean(bool b) : b_(b) {
    }
    bool GetBool() const {
        return b_;
    }
    std::shared_ptr<Object> Eval(std::shared_ptr<Scope>&) override {
        return shared_from_this();
    }
    std::string Stringify() override {
        return b_ ? "#t" : "#f";
    }

private:
    bool b_;
};

class Cell : public Object {
public:
    Cell(std::shared_ptr<Object> first, std::shared_ptr<Object> second)
            : first_(first), second_(second) {
    }

    std::shared_ptr<Object>& GetFirst() {
        return first_;
    }
    std::shared_ptr<Object>& GetSecond() {
        return second_;
    }

    std::shared_ptr<Object> Eval(std::shared_ptr<Scope>&) override {
        throw NameError("Cannot Evaluate a pair");
    }

    std::string Stringify() override {
        if (GetFirst() == nullptr && GetSecond() == nullptr) {  // special case for (())
            return "(())";
        }
        return StringifyImpl(true);
    }

private:
    std::shared_ptr<Object> first_;
    std::shared_ptr<Object> second_;

    std::string StringifyImpl(bool need_paren) {
        std::string cell_string;
        auto head = GetFirst();
        auto tail = GetSecond();

        if (need_paren) {
            cell_string += '(';
        }

        if (Is<Cell>(head)) {
            cell_string += As<Cell>(head)->StringifyImpl(true);  // additional layer of parentheses
        } else {
            cell_string += head->Stringify();
        }

        if (Is<Cell>(tail)) {
            cell_string += ' ';
            cell_string += As<Cell>(tail)->StringifyImpl(false);
        } else if (Is<Number>(tail) || Is<Boolean>(tail)) {
            cell_string += " . ";
            cell_string += tail->Stringify();
        }

        if (need_paren) {
            cell_string += ')';
        }
        return cell_string;
    }
};

class Function : public Object {
public:
    virtual std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                          const std::vector<std::shared_ptr<Object>>& objects) = 0;

    std::shared_ptr<Object> Eval(std::shared_ptr<Scope>&) override {
        throw RuntimeError("No Eval() for Function");
    }

    std::string Stringify() override {
        throw RuntimeError("No Stringify for Function");
    }
};

///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<Object> Evaluate(std::shared_ptr<Object> cell, std::shared_ptr<Scope>& scope);
void ArgsToVector(std::vector<std::shared_ptr<Object>>& vec, std::shared_ptr<Object> cell,
                  const std::string& name, std::shared_ptr<Scope>& scope);
std::string Serialize(std::shared_ptr<Object> cell);

///////////////////////////////////////////////////////////////////////////////

// Functions

class Lambda : public Object {
public:
    Lambda(std::shared_ptr<Scope>& scope, const std::vector<std::string>& args,
           const std::vector<std::shared_ptr<Object>>& evals)
            : args_(args), evals_(evals) {
        local_scope_ = std::make_shared<Scope>();
        local_scope_->GetParentScope() = scope;
    }

    // DO SCOPE

    std::shared_ptr<Object> EvalLambda(std::vector<std::shared_ptr<Object>>& objects) {

        if (objects.size() != args_.size()) {
            throw SyntaxError("bad lambda arguments");
        }
        for (size_t i = 0; i < args_.size(); ++i) {
            local_scope_->DefineSymbol(args_[i], objects[i]);
        }
        for (size_t i = 0; i < evals_.size() - 1; ++i) {
            Evaluate(evals_[i], local_scope_);
        }
        return Evaluate(evals_.back(), local_scope_);
    }

    std::shared_ptr<Object> Eval(std::shared_ptr<Scope>&) override {
        throw RuntimeError("No Eval for Lambda");
    }

    std::string Stringify() override {
        throw RuntimeError("No Stringify for Lambda");
    }

private:
    std::shared_ptr<Scope> local_scope_;
    std::vector<std::string> args_;
    std::vector<std::shared_ptr<Object>> evals_;
};

class LambdaCreator : public Object {
public:
    LambdaCreator(std::shared_ptr<Scope>& scope, const std::vector<std::string>& args,
                  const std::vector<std::shared_ptr<Object>>& evals)
            : lambda_args_(args), lambda_evals_(evals) {
        lambda_parent_scope_ = scope;
    }

    std::shared_ptr<Lambda> CreateLambda() {
        return std::make_shared<Lambda>(lambda_parent_scope_, lambda_args_, lambda_evals_);
    }

    std::shared_ptr<Object> Eval(std::shared_ptr<Scope>&) override {
        throw RuntimeError("No Eval for LambdaCreator");
    }

    std::string Stringify() override {
        throw RuntimeError("No Stringify for LambdaCreator");
    }

private:
    std::shared_ptr<Scope> lambda_parent_scope_;
    std::vector<std::string> lambda_args_;
    std::vector<std::shared_ptr<Object>> lambda_evals_;
};

class Quote : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>&) override {
        throw NameError("Apply should not be invoked for function 'quote'");
    }
};

class IsNumber : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.size() != 1) {
            throw RuntimeError("number? function must have only 1 argument");
        }
        return std::make_shared<Boolean>(Is<Number>(objects.front()));
    }
};

class Equal : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        ASSERT_ALL_NUMBERS(objects)
        for (size_t i = 1; i < objects.size(); ++i) {
            if (As<Number>(objects[i - 1])->GetValue() != As<Number>(objects[i])->GetValue()) {
                return std::make_shared<Boolean>(false);
            }
        }
        return std::make_shared<Boolean>(true);
    }
};

class Greater : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        ASSERT_ALL_NUMBERS(objects)
        for (size_t i = 1; i < objects.size(); ++i) {
            if (As<Number>(objects[i - 1])->GetValue() <= As<Number>(objects[i])->GetValue()) {
                return std::make_shared<Boolean>(false);
            }
        }
        return std::make_shared<Boolean>(true);
    }
};

class Less : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        ASSERT_ALL_NUMBERS(objects)
        for (size_t i = 1; i < objects.size(); ++i) {
            if (As<Number>(objects[i - 1])->GetValue() >= As<Number>(objects[i])->GetValue()) {
                return std::make_shared<Boolean>(false);
            }
        }
        return std::make_shared<Boolean>(true);
    }
};

class GreaterOrEqual : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        ASSERT_ALL_NUMBERS(objects)
        for (size_t i = 1; i < objects.size(); ++i) {
            if (As<Number>(objects[i - 1])->GetValue() < As<Number>(objects[i])->GetValue()) {
                return std::make_shared<Boolean>(false);
            }
        }
        return std::make_shared<Boolean>(true);
    }
};

class LessOrEqual : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        ASSERT_ALL_NUMBERS(objects)
        for (size_t i = 1; i < objects.size(); ++i) {
            if (As<Number>(objects[i - 1])->GetValue() > As<Number>(objects[i])->GetValue()) {
                return std::make_shared<Boolean>(false);
            }
        }
        return std::make_shared<Boolean>(true);
    }
};

class Add : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        ASSERT_ALL_NUMBERS(objects)
        int64_t result = 0;
        for (const auto& obj : objects) {
            result += As<Number>(obj)->GetValue();
        }
        return std::make_shared<Number>(result);
    }
};

class Multiply : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        ASSERT_ALL_NUMBERS(objects)
        int64_t result = 1;
        for (const auto& obj : objects) {
            result *= As<Number>(obj)->GetValue();
        }
        return std::make_shared<Number>(result);
    }
};

class Sub : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.empty()) {
            throw RuntimeError("At least one argument must be passed to function '-'");
        }
        ASSERT_ALL_NUMBERS(objects)
        int64_t result = As<Number>(objects.front())->GetValue();
        for (size_t i = 1; i < objects.size(); ++i) {
            result -= As<Number>(objects[i])->GetValue();
        }
        return std::make_shared<Number>(result);
    }
};

class Divide : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.empty()) {
            throw RuntimeError("At least one argument must be passed to function '/'");
        }
        ASSERT_ALL_NUMBERS(objects)
        int64_t result = As<Number>(objects.front())->GetValue();
        for (size_t i = 1; i < objects.size(); ++i) {
            result /= As<Number>(objects[i])->GetValue();
        }
        return std::make_shared<Number>(result);
    }
};

class Abs : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.size() != 1) {
            throw RuntimeError("Abs function must have only 1 argument");
        }
        ASSERT_ALL_NUMBERS(objects)
        return std::make_shared<Number>(std::abs(As<Number>(objects.front())->GetValue()));
    }
};

class Max : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.empty()) {
            throw RuntimeError("At least one argument must be passed to function 'max'");
        }
        ASSERT_ALL_NUMBERS(objects)
        int64_t result = As<Number>(objects.front())->GetValue();
        for (size_t i = 1; i < objects.size(); ++i) {
            result = std::max(result, As<Number>(objects[i])->GetValue());
        }
        return std::make_shared<Number>(result);
    }
};

class Min : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.empty()) {
            throw RuntimeError("At least one argument must be passed to function 'min'");
        }
        ASSERT_ALL_NUMBERS(objects)
        int64_t result = As<Number>(objects.front())->GetValue();
        for (size_t i = 1; i < objects.size(); ++i) {
            result = std::min(result, As<Number>(objects[i])->GetValue());
        }
        return std::make_shared<Number>(result);
    }
};

class IsBoolean : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.size() != 1) {
            throw RuntimeError("number? function must have only 1 argument");
        }
        return std::make_shared<Boolean>(Is<Boolean>(objects.front()));
    }
};

class BooleanNot : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.size() != 1) {
            throw RuntimeError("number? function must have only 1 argument");
        }
        if (Is<Boolean>(objects.front()) && !As<Boolean>(objects.front())->GetBool()) {
            return std::make_shared<Boolean>(true);
        }
        return std::make_shared<Boolean>(false);
    }
};

class BooleanAnd : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.empty()) {
            return std::make_shared<Boolean>(true);
        }
        for (const auto& obj : objects) {
            if (Is<Cell>(obj) && As<Symbol>(As<Cell>(obj)->GetFirst())->GetName() != "quote") {
                throw NameError("Invalid name of the function");
            }
            if (Is<Boolean>(obj) && !As<Boolean>(obj)->GetBool()) {
                return obj;
            }
        }
        if (Is<Cell>(objects.back())) {  // case of last argument being passed as a valid quote-pair
            auto tail = As<Cell>(objects.back())->GetSecond();
            if (Is<Cell>(tail)) {
                return As<Cell>(tail)->GetFirst();
            }
            return tail;
        }
        return objects.back();
    }
};

class BooleanOr : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.empty()) {
            return std::make_shared<Boolean>(false);
        }
        for (const auto& obj : objects) {
            if (Is<Cell>(obj) && As<Symbol>(As<Cell>(obj)->GetFirst())->GetName() != "quote") {
                throw NameError("Invalid name of the function");
            }
            if (!Is<Boolean>(obj) || As<Boolean>(obj)->GetBool()) {
                return obj;
            }
        }
        if (Is<Cell>(objects.back())) {  // case of last argument being passed as a valid quote-pair
            auto tail = As<Cell>(objects.back())->GetSecond();
            if (Is<Cell>(tail)) {
                return As<Cell>(tail)->GetFirst();
            }
            return tail;
        }
        return objects.back();
    }
};

class IsPair : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        auto cell = objects.front();
        size_t size = 0;
        while (cell != nullptr) {
            if (Is<Number>(cell) || Is<Boolean>(cell)) {
                ++size;
                break;
            }
            if (As<Cell>(cell)->GetFirst() != nullptr) {
                ++size;
            }
            cell = As<Cell>(cell)->GetSecond();
        }
        return std::make_shared<Boolean>((size == 2));
    }
};

class IsNull : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        return std::make_shared<Boolean>((objects.front() == nullptr));
    }
};

class IsList : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        auto cell = objects.front();
        if (cell == nullptr) {
            return std::make_shared<Boolean>(true);
        }
        while (cell != nullptr) {
            if (Is<Number>(cell) || Is<Boolean>(cell)) {
                return std::make_shared<Boolean>(false);
            }
            cell = As<Cell>(cell)->GetSecond();
        }
        return std::make_shared<Boolean>(true);
    }
};

class Cons : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.size() != 2) {
            throw RuntimeError("cons function must have exactly 2 arguments");
        }
        return std::make_shared<Cell>(objects.front(), objects.back());
        //        throw RuntimeError("cons function arguments must be numbers of boolean");
    }
};

class Car : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        auto cell = objects.front();
        if (cell == nullptr) {
            throw RuntimeError("Cannot use car function on empty list");
        }
        if (!Is<Cell>(cell)) {
            throw RuntimeError("You can only use 'car' on lists");
        }
        return As<Cell>(cell)->GetFirst();
    }
};

class Cdr : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        auto cell = objects.front();
        if (cell == nullptr) {
            throw RuntimeError("Cannot use cdr function on empty list");
        }
        if (!Is<Cell>(cell)) {
            throw RuntimeError("You can only use 'cdr' on lists");
        }
        return As<Cell>(cell)->GetSecond();
    }
};

class CreateList : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        std::shared_ptr<Object> result_cell = nullptr;
        std::shared_ptr<Cell> cur_cell{};
        for (const auto& obj : objects) {
            if (result_cell == nullptr) {
                cur_cell = std::make_shared<Cell>(obj, nullptr);
                result_cell = cur_cell;
            } else {
                auto tail_cell = std::make_shared<Cell>(obj, nullptr);
                cur_cell->GetSecond() = tail_cell;
                cur_cell = tail_cell;
            }
        }
        return result_cell;
    }
};

class ListRef : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.size() != 2) {
            throw RuntimeError("list-ref function must take 2 arguments : list and index");
        }
        auto cell = objects.front();
        auto number = objects.back();
        if (!Is<Number>(number)) {
            throw RuntimeError("Second argument of list-ref function must be a number");
        }
        size_t index = As<Number>(number)->GetValue();
        while (index != 0) {
            if (!Is<Cell>(cell)) {
                throw RuntimeError("Invalid list");
            }
            if (As<Cell>(cell)->GetSecond() == nullptr) {
                throw RuntimeError("Invalid index");
            }
            --index;
            cell = As<Cell>(cell)->GetSecond();
        }
        return As<Cell>(cell)->GetFirst();
    }
};

class ListTail : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.size() != 2) {
            throw RuntimeError("list-ref function must take 2 arguments : list and index");
        }
        auto cell = objects.front();
        auto number = objects.back();
        if (!Is<Number>(number)) {
            throw RuntimeError("Second argument of list-ref function must be a number");
        }
        size_t index = As<Number>(number)->GetValue();
        while (index != 0) {
            if (cell == nullptr) {
                throw RuntimeError("Invalid index");
            }
            if (!Is<Cell>(cell)) {
                throw RuntimeError("Invalid list");
            }
            --index;
            cell = As<Cell>(cell)->GetSecond();
        }
        return cell;
    }
};

class IsSymbol : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.size() != 1) {
            throw RuntimeError("symbol? function must have only 1 argument");
        }
        return std::make_shared<Boolean>(Is<Symbol>(objects.front()));
    }
};

class Define : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>& scope,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.size() < 2) {
            throw SyntaxError("define function must take 2 arguments : variable and its value");
        }
        auto variable = objects.front();
        if (!Is<Symbol>(variable)) {
            throw SyntaxError("The first argument of function define is the name");
        }
        if (objects.size() > 2) {
            if (!Is<Lambda>(objects[1])) {
                throw SyntaxError("define function must take 2 arguments : variable and its value");
            }
            auto lambda = objects[1];
            std::vector<std::shared_ptr<Object>> lambda_objects;
            for (size_t i = 2; i < objects.size(); ++i) {
                lambda_objects.push_back(objects[i]);
            }
            auto value = As<Lambda>(lambda)->EvalLambda(lambda_objects);
            scope->DefineSymbol(As<Symbol>(variable)->GetName(), value);
            return nullptr;
        }
        auto value = objects.back();
        if (Is<Symbol>(value) && scope->AllContains(As<Symbol>(value)->GetName())) {
            value = scope->AllResolveSymbol(As<Symbol>(value)->GetName());
        }
        scope->DefineSymbol(As<Symbol>(variable)->GetName(), value);
        return nullptr;
    }
};

class Set : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>& scope,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.size() != 2) {
            throw SyntaxError("set! function must take 2 arguments : variable and its value");
        }
        auto variable = objects.front();
        auto value = objects.back();
        if (!Is<Symbol>(variable)) {
            throw SyntaxError("The first argument of function set! is the name");
        }
        if (!scope->AllContains(As<Symbol>(variable)->GetName())) {
            throw NameError(As<Symbol>(variable)->GetName());
        }
        if (Is<Symbol>(value) && scope->AllContains(As<Symbol>(value)->GetName())) {
            value = scope->AllResolveSymbol(As<Symbol>(value)->GetName());
        }
        scope->SetDefineSymbol(As<Symbol>(variable)->GetName(), value);
        return nullptr;
    }
};

class If : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>& scope,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.size() != 2 && objects.size() != 3) {
            throw SyntaxError("Invalid number of arguments for function 'if'");
        }
        auto condition = objects.front();
        auto true_branch = objects[1];
        auto false_branch = (objects.size() == 3) ? objects[2] : nullptr;
        if (Is<Boolean>(condition) && !As<Boolean>(condition)->GetBool()) {  // if false
            if (false_branch == nullptr) {
                return false_branch;
            }
            return Evaluate(false_branch, scope);
        } else {
            return Evaluate(true_branch, scope);
        }
    }
};

class SetCar : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.size() != 2) {
            throw SyntaxError("2 arguments are required for function 'set-car!'");
        }
        auto cell = objects.front();
        auto value = objects.back();
        if (cell == nullptr) {
            throw RuntimeError("Cannot use car function on empty list");
        }
        if (!Is<Cell>(cell)) {
            throw RuntimeError("You can only use 'car' on lists");
        }
        As<Cell>(cell)->GetFirst() = value;
        return nullptr;
    }
};

class SetCdr : public Function {
    std::shared_ptr<Object> Apply(std::shared_ptr<Scope>&,
                                  const std::vector<std::shared_ptr<Object>>& objects) override {
        if (objects.size() != 2) {
            throw SyntaxError("2 arguments are required for function 'set-cdr!'");
        }
        auto cell = objects.front();
        auto value = objects.back();
        if (cell == nullptr) {
            throw RuntimeError("Cannot use car function on empty list");
        }
        if (!Is<Cell>(cell)) {
            throw RuntimeError("You can only use 'car' on lists");
        }
        As<Cell>(cell)->GetSecond() = value;
        return nullptr;
    }
};