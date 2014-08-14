Quala: Type Qualifiers for LLVM/Clang
=====================================

<img src="qτ.png" align="right" width="202" height="132" alt="qτ the koala">

This is an experiment in adding overlay type systems to LLVM and Clang, inspired by Java 8's [JSR-308][] and the [Checker Framework][].

[Checker Framework]: http://types.cs.washington.edu/checker-framework/
[JSR-308]: http://www.jcp.org/en/jsr/detail?id=308


## Motivation

User-customizable type systems make it possible to add optional checks to a language without hacking the compiler. The world is full of great ideas for one-off type systems that help identify specific problems---SQL injection, say---but it's infeasible to expect all of these to be integrated into a language spec or a compiler. Who would want to deal with hundreds of type system extensions they're not really using?

Java's JSR-308 invented a clever solution to this problem: make type systems *pluggable*. Add support to the language for arbitrary type annotations and then let users load in libraries that provide typing rules for whatever system they can dream up.

I want to port this idea to C and C++ with a twist: I need custom type qualifiers to be visible in an intermediate representation so they're available to heavyweight compiler machinery. This is an attempt to permit type qualifiers and custom type checkers in Clang that record their types as metadata in the resultant LLVM IR.


## Building

I'm tracking the unreleased 3.5 branches of LLVM and Clang at the moment. They're included in submodules to indicate the revision I've tested with.
LLVM is stock and unmodified; Clang is a [patched version](https://github.com/sampsyo/clang-quala) that adds a new type kind called `AnnotatedType`.

You can get started by cloning this repo with its submodules and typing `make llvm` to build LLVM and Clang. This uses [CMake][] and [Ninja][], which is my favorite route to a working toolchain.

Then `make` will build the Clang plugin. It's somewhat verbose to get the Clang arguments right to load the plugin, so the `bin/tqclang` script helps with that.

[Ninja]: http://martine.github.io/ninja/
[CMake]: http://www.cmake.org/


## Status

There is a mostly complete frontend type-checking framework. The [TypeAnnotations.h][] template header contains a "library" for writing type systems.

As an example, [TaintTracking.cpp][] implements a simple information flow type system as a Clang plugin. The definition of the type system itself is relatively straightforward because the library takes care most of the boilerplate.

You can see a tiny example of the information flow system in [simple.c][]. Compiling that file emits type errors like this:

    no.c:7:3: error: tainted incompatible with unannotated
      y = x;
      ^~~~

The code generation aspects are not yet implemented.

[simple.c]: https://github.com/sampsyo/quala/blob/master/examples/tainting/test/simple.c
[TaintTracking.cpp]: https://github.com/sampsyo/quala/blob/master/examples/tainting/TaintTracking.cpp
[TypeAnnotations.h]: https://github.com/sampsyo/quala/blob/master/TypeAnnotations.h
