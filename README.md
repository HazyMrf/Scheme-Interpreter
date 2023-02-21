# Scheme-Interpreter

**Here goes the implementation of interpreter for LISP-like programming language, namely a certain subset of Scheme.**

Language itself consists of:
- Primitive types include integers, booleans, and symbols (identifiers).
- Complex types: lists and pairs.
- Variables with local scope.
- Functions ans lambda-functions.

Some examples (here `=>` separates an expression from the result of its evaluation )

```
    1 => 1
    (+ 1 2) => 3
    (+ 1 (+ 3 4 5)) => 13
    (define test (lambda (x) (set! x (* x 2)) (+ 1 x)) ) => (some lambda)
    (define x '(1 . 2)) => (some pair)
```
You can see more examples in the folder `tests`

## Details of the implementation
The execution is split in three stages:

**Tokenization** - converts the program text into a sequence of individual lexical units.

**Syntax Analysis** - It transforms the sequence of tokens into an AST(https://en.wikipedia.org/wiki/Abstract_syntax_tree). In Lisp-like programming languages, the AST is represented as lists..

**Computation** - performs a recursive traversal of the program's AST, applying a set of rules to transform it along the way.

## Additional Info

* Introduction to Scheme: https://www.youtube.com/watch?v=AqBxU-Zmx00 explains the basics of a programming language.

* Книга [Build Your Own Lisp](http://www.buildyourownlisp.com/) разбирает детали реализации интерпретатора на языке C.

* Книга [Crafting Interpreters](http://craftinginterpreters.com/) разбирает реализацию интерпретатора для более сложного языка, чем LISP.
