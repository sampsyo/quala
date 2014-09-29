Quala: Type Qualifiers for LLVM/Clang
=====================================

<img src="qτ.png" align="right" width="202" height="132" alt="qτ the koala">

This is an experiment in adding overlay type systems to LLVM and Clang, inspired by Java 8's [JSR-308][] and the [Checker Framework][].

[Checker Framework]: http://types.cs.washington.edu/checker-framework/
[JSR-308]: http://www.jcp.org/en/jsr/detail?id=308

User-customizable type systems make it possible to add optional checks to a language without hacking the compiler. The world is full of great ideas for one-off type systems that help identify specific problems—SQL injection, say—but it's infeasible to expect all of these to be integrated into a language spec or a compiler. Who would want to deal with hundreds of type system extensions they're not really using?

Java's JSR-308 invented a clever solution to this problem: make type systems *pluggable*. Add support to the language for arbitrary type annotations and then let users load in libraries that provide typing rules for whatever system they can dream up.

I want to port this idea to C and C++ with a twist: I need custom type qualifiers to be visible in an intermediate representation so they're available to heavyweight compiler machinery. This is an attempt to permit type qualifiers and custom type checkers in Clang that record their types as metadata in the resultant LLVM IR.


## Building

This repository includes LLVM and Clang as submodules to make building against the right version easy. Clone with `--recurse-submodules` and type `make llvm` to build Clang itself. This Make target uses [CMake][] and [Ninja][], which is my favorite route to a working toolchain.

(Specifically, Quala is based on version 3.5 of LLVM and Clang (the most recent release as of this writing). LLVM is stock and unmodified; Clang is a [patched version][clang-quala] that adds a new type kind called `AnnotatedType`.)

[Ninja]: http://martine.github.io/ninja/
[CMake]: http://www.cmake.org/
[clang-quala]: https://github.com/sampsyo/clang-quala


## Example Type Systems

There are two example typesystems currently: *tainting* and *nullness* (both inspired by equivalents in the [Checker Framework][]). To build either one, cd to `examples/tainting/` or `examples/nullness/` and type `make`. Or type `make test` to check that it's actually working.

The type systems come with wrapper scripts that invoke Clang with the right arguments to load the plugin and enable the checker. Use these scripts to compile your own code, sit back, and enjoy the type-checking show.

### Tainting

The first example implements [information flow tracking][ift], which can prevent some kinds of security vulnerabilities. The type system tracks *tainted* values and emits errors when they can influence untainted values. For example, you could use this type system to ensure that no user input flows to SQL statements, thereby preventing SQL injection bugs.

As with any Quala type system, you want to define a macro that encapsulates the type annotation:

    #define TAINTED __attribute__((type_annotate("tainted")))

(This might go in a `tainting.h` header, for example, but it doesn't have to.) Then you can write:

   TAINTED int dangerous;
   int safe;
   safe = dangerous;  // BAD

and get an error on line 3.

To suppress errors, define an `ENDORSE` macro:

    #define ENDORSE(e) __builtin_annotation((e), "endorse")

and then this assignment will not emit an error:

    safe = ENDORSE(dangerous);

The type system also prevents tainted values from being used in conditions, which conservatively prevents [implicit flows][impflow].

[impflow]: http://en.wikipedia.org/wiki/Information_flow_(information_theory)#Explicit_Flows_and_Side_Channels
[ift]: http://en.wikipedia.org/wiki/Information_flow_(information_theory)

### Nullness

The *nullness* type system helps catch potential null-pointer dereferences at compile time. It can be thought of as a sound version of the [Clang analyzer][]'s null-dereference checker (that needs a little help from you).

By default, all pointers are considered *non-null* and the type system emits a warning whenever you try to nullify one:

    int *p = 0;  // WARNING
    int *q = nullptr;  // EXTRA-SPECIAL C++11 WARNING

To allow a pointer to be null, you have to mark it as *nullable*:

    #define NULLABLE __attribute__((type_annotate("nullable")))
    int * NULLABLE p = 0;  // ok

Note that declaration is careful to declare a *nullable pointer to an int*, not a pointer to a nullable int—which wouldn't make much sense. One of Quala's strengths (over the Clang analyzer's `nonnull` annotation, for example) is that you can put annotations exactly where you mean them. For example:

    int ** NULLABLE p; // nullable pointer to a non-null pointer
    int * NULLABLE *q; // non-null pointer to a nullable pointer

You can also use annotations anywhere that types go, not just on variable declarations:

    typedef int * NULLABLE nullable_int_ptr;

[Clang analyzer]: http://clang-analyzer.llvm.org/available_checks.html


## Status

The [modifications to Clang][clang-quala] are relatively minor; there's one new type kind and one new annotation. There's some nonzero chance that these could land in Clang trunk. (Let me know if you have connections!)

There is a mostly complete frontend type-checking framework. The [TypeAnnotations.h][] template header contains a "library" for writing type systems.

The code generation aspects are not yet implemented.

Quala is by [Adrian Sampson][] and made available under the [MIT license][].

[Adrian Sampson]: http://homes.cs.washington.edu/~asampson/
[MIT license]: http://opensource.org/licenses/MIT
[TypeAnnotations.h]: https://github.com/sampsyo/quala/blob/master/TypeAnnotations.h
