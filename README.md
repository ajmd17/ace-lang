# Ace Programming Language

Ace is a dynamic, optionally typed programming language created in C++.
It is inspired by languages such as C++, JavaScript, Rust and Ruby.


See *proto* branch to see some cool stuff. Experimental.


## Philosophy

To give programmers the freedom to learn by making mistakes, explore programming in a way that works best for them, and to provide them with the ability to not only use existing features, but to create new ones for themselves.

## Documentation

- [Installation](./docs/installation.md)
- [Syntax and Usage](./docs/syntax.md)
- [Built-in Modules](./docs/built-in-modules.md)
- [Example Code](./examples)

## Roadmap 2017

* Finish block-expressions

* meta-scripting (the ability to control the compiler)
    * an "eval"-type compile-time mixin generator (not runtime, like most evals). used for pasting code into the above scope. `$mixin(...)`? `$(...)`?

* type-traits - contracts/interfaces that may be resolved at compile time or runtime. just a function that resolves to a boolean
    * `Callable : Trait = t => t is Function || t has '$invoke'`
    * Builtin type traits such as `Number`:
        * `Number : Trait = t => t is Int || t is Float;`

* mixins - Macro-like reusable code that can be used for source code templating.
    * The `mixin` keyword is used when using a mixin as well as defining one, for clarity. Example:
        * `mixin max(a, b) = "#{a} > #{b} ? #{a} : #{b}";`
        * `max_num : Int = mixin max(5, 6); // max_num == 6`

* type-check-expressions (`is` and `as`): `is` scans for types and checks, returning a boolean. If no types found, calls a `Trait` expression. `as` runs an `is` check and returns `null` if the `is` check returned `false`. if the `is` check returned `true`, it casts by using a generic member: `as<T>()`

* type-objects (prototype inheritance system): types are variables that can be loaded at runtime

* generic-expressions: Remove generic types/params on the current type system, and make a new system that applies to any variable (now that types are variables themselves with type-objects). Generic expressions just create placeholder variables before visiting the assignment expression. The hard part is how do we fill in the right parameters in order. Example:
    * `PI <T : Number> : Const <T> = 3.141592654 as T;`
    * `pi_int : Int = PI <Int>; // OK, pi_int is 3`
    * `pi_str : String = PI <String>; // Error, not 'Number'`

* lambda-expressions
    * for functions. simple syntax with `f: Function = (a, b) => a + b;`
    * for builtin language features: `while x > y => x--;`

* structs (based on block-expressions)
    * Simple JS Object/Hash type structures without an explicit type, relying on type-traits for type checking.
    * Example: `s: Struct = { i: Int = 0; j: Float = 3.1415; }`

* actions (pipes/event handlers)

* boolean-literals
    * `yes` and `no`, `on` and `off` (they're all synonymous)
    * Default to be printed can be set with a flag

        * `$meta { $set('bool.default', 'on/off'); }`

* runtime-typechecking
    * Applied by default for typed function arguments. A simple assertion which throws an exception. May be disabled in release mode or with a flag:
    * `$meta { $set('runtime.typechecking', off); }`

* self-hosting: parts of the language, written in Ace itself to start down the path of allowing a super dynamic and bootstrappable language.
