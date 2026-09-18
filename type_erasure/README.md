## Type Erasure

A logger with interchangeable output sinks is used to show the usage of type
erasure. Its high-level structure is the following:

```
             User application
                    │
                    │ holds Logger by value
                    ▼
          ┌─────────────────────┐
          │       Logger        │
          │ unique_ptr<Base>    │  <- public wrapper, no template parameter
          └─────────────────────┘
                    │
                    │ points to
                    ▼
          ┌─────────────────────┐
          │     LoggerBase      │  <- abstract, declares log()
          └─────────────────────┘
                    ▲
                    │ implements
          ┌─────────────────────┐
          │ LoggerWrpper<Sink>  │  <- holds a Sink, forwards log()
          └─────────────────────┘
                    │
                    │ calls sink.log(level, message)
                    ▼
            ConsoleSink, FileSink   <- unrelated types, no base class
```

`Logger` has a constructor template, so it accepts any type satisfying
`SinkConcept`, but `Logger` itself is not a template. The sink type appears in
the constructor and nowhere else, so two `Logger` objects holding different sink
types have the same type and can be assigned to each other. A minimum
`LogLevel` is stored on `Logger` itself; messages below it are dropped before
the sink is called.

Type erasure is a constructor template plus a non-virtual public interface plus
the combination of three design patterns: **external polymorphism** (the
inheritance hierarchy is added from outside, so the sinks need not derive from
anything), **bridge** (the wrapper holds a pointer to the implementation), and
**prototype** (`LoggerBase::clone()` copies the stored sink, so copying a
`Logger` yields an independent object rather than a shared one).

### What is the use of type erasure?

Ordinarily, calling `log()` on an arbitrary sink type requires either a template
parameter on every class that stores a sink, or a common base class the sinks
derive from. The first spreads the sink type through the whole program and rules
out changing sinks at runtime. The second requires the author of each sink to
include the logger's header and derive from its base class, so every sink type
depends on the logging library.

Type erasure removes both requirements. `ConsoleSink` and `FileSink` in
[sinks.hpp](sinks.hpp) derive from nothing and do not include
[logger.hpp](logger.hpp). The dependency runs only one way: `Logger` requires
that a sink has a `log(LogLevel, std::string_view)` member, and the sink knows
nothing about `Logger`. A sink from a library whose source cannot be modified
works the same way.

The requirement is stated as a C++20 concept rather than left implicit:

```cpp
template <typename Sink>
concept SinkConcept = std::copyable<Sink> && requires(Sink sink, LogLevel level, std::string_view message) {
    sink.log(level, message);
};
```

A type that does not satisfy it is rejected at the `Logger` constructor, rather
than producing an error inside `LoggerWrpper`. `std::copyable` is required
because `clone()` copy-constructs the stored sink.

### Adding a type versus adding an operation

A design can grow in two ways: a new type can be introduced, or a new operation
on the existing types can be introduced. Each technique makes one of these cheap
and the other expensive.

| | Add a type | Add an operation |
| --- | --- | --- |
| `std::variant` | expensive | cheap |
| virtual base class | cheap | expensive |
| type erasure | cheap | expensive |

A `std::variant` lists every type it can hold at the point of declaration:

```cpp
using Sink = std::variant<ConsoleSink, FileSink>;
```

Adding `SyslogSink` means editing that declaration. Every translation unit
including it recompiles, and every `std::visit` that handled exactly two cases
fails to compile until a third is added. A sink from a library the author does
not control cannot be added at all, and neither can one supplied at runtime by a
shared library.

Adding an operation to a `std::variant` costs nothing, because the operation is
a free function written in a separate file:

```cpp
void flush(Sink& s) {
    std::visit(overloaded{
        [](ConsoleSink&)   { std::cout.flush(); },
        [](FileSink& f)    { f.reopen(); },
    }, s);
}
```

`ConsoleSink` never declared that it supports `flush`. Any number of such
functions can be added from anywhere without modifying the sinks or the variant.

A virtual base class reverses both costs. A new derived sink can be written in a
separate library and used by existing code without recompiling it, but adding a
virtual function to the base class forces every derived class to implement it
and changes the layout of the virtual function table. That table, usually called
the vtable, is an array of function pointers the compiler generates for each
polymorphic class; a virtual call reads the pointer at a fixed position in it.
Inserting a function shifts the positions after it, so a client compiled against
the old layout calls through the wrong entry.

Type erasure has the same cost profile as the virtual base class. Adding
`flush()` to this demo means adding a pure virtual function to `LoggerBase`, an
override to `LoggerWrpper`, a forwarding function to `Logger`, and a second
requirement to `SinkConcept`. All are in [logger.hpp](logger.hpp), so everything
including it recompiles. There is a further cost the virtual base class does not
have: every existing sink must now provide `flush` as well, so `Logger` code that
previously compiled stops compiling.

Adding a sink type costs nothing, and the sink is not modified:

```cpp
struct SyslogSink {                                  // no base class
    void log(LogLevel, std::string_view);            // nothing included from logger.hpp
};

logger = Logger{SyslogSink{}};                       // compiles
```

Two points qualify the cost table above. Neither changes which of the two
kinds of growth is cheap, but both affect how far the technique reaches:

* **The list of operations is fixed, but their implementations are not.** This
  demo dispatches through the member `m_sink.log(...)`, so a sink must have a
  member function of that name, which a type whose source cannot be modified may
  not have. Dispatching through an unqualified call to a free function instead
  removes that requirement, because of argument-dependent lookup: when a function
  is called without naming its namespace, the compiler also searches the
  namespaces in which the argument types are declared. This is the same rule that
  makes an unqualified `swap(a, b)` find `std::swap` when `a` and `b` are
  `std::string`. Writing `log_to(m_sink, level, message)` in `LoggerWrpper` lets
  the author of a sink, or anyone able to add a function to that sink's
  namespace, supply the operation from outside:

  ```cpp
  namespace thirdparty {
      struct Journal { void emit(int, const char*); };   // cannot be modified

      // but a free function can be added in the same namespace
      void log_to(Journal& j, LogLevel level, std::string_view msg) {
          j.emit(static_cast<int>(level), msg.data());
      }
  }

  Logger logger{thirdparty::Journal{}};   // Journal is untouched
  ```

  `LoggerWrpper` does not know that `thirdparty` exists; the call is resolved
  because its first argument is declared there. The name has to be something
  other than `log`, since an unqualified `log` also finds the one from `<cmath>`.
  The set of operations `LoggerBase` declares is just as fixed either way, so the
  cost of adding one does not change.
* **The operation set need not be a single fixed interface.** Libraries such as
  [dyno](https://github.com/ldionne/dyno) and
  [folly::Poly](https://github.com/facebook/folly/blob/main/folly/docs/Poly.md)
  compose it from separate declarations, so a new operation is declared on its
  own and combined rather than added to an existing interface. This moves the
  cost rather than removing it, at a considerable increase in complexity.

Making both cheap at once is the problem of open multimethod dispatch, which C++
does not support directly; implementations use a runtime registry keyed on
`std::type_index` or a similar mechanism.

### Relation to the other demos in this repository

`Logger` holds a single `std::unique_ptr`, so its size does not change when the
set of sinks or the implementation changes. This is the property
[pimpl/](../pimpl/) gives a concrete class, applied here to a polymorphic
interface. The alternatives change the application binary interface in different
places. The application binary interface, or ABI, is the set of details two
separately compiled pieces of code must agree on to call each other at runtime:
the size and layout of types, the position of entries in a vtable, and the
mangled names of symbols. Adding an alternative to a `std::variant` changes its
size, and adding a virtual function to a base class changes the vtable; either
makes an already compiled client disagree with the library it is loaded against.
The C layer in [hourglass/](../hourglass/) avoids both by exposing neither across
the library boundary.

### Layout

| Path | Role |
| --- | --- |
| [logger.hpp](logger.hpp) | `SinkConcept`, the `LoggerBase`/`LoggerWrpper` hierarchy, and the public `Logger` |
| [sinks.hpp](sinks.hpp) | `ConsoleSink` and `FileSink`, which derive from nothing |
| [sinks.cpp](sinks.cpp) | Sink implementations |
| [log_level.hpp](log_level.hpp) | `LogLevel` enumeration, ordering, and `to_string` |
| [main.cpp](main.cpp) | Demo application; stores two unrelated sink types in one `Logger` |
| [CMakeLists.txt](CMakeLists.txt) | Builds the sink library and the demo executable |

### Building and running

```bash
cmake -S . -B build && cmake --build build
./build/type_erasure
```

Output on standard output:

```
[INFO] Application started
[ERROR] Configuration file not found
```

`Connecting to database` is Debug and is dropped by the console logger's Info
threshold. The remaining messages go to `application.log`; `Switched to file
logging` is Info and is dropped by the Error threshold, so both
`[ERROR] Connection failed` and `[ERROR] Retrying connection` are written.

### External resources

| Resource | Why |
| --- | --- |
| [Inheritance Is The Base Class of Evil](https://www.youtube.com/watch?v=bIhUE5uUFOA) — Sean Parent, GoingNative 2013 | The original talk; builds the technique from scratch in about 24 minutes |
| [What is Type Erasure?](https://quuxplusone.github.io/blog/2019/03/18/what-is-type-erasure/) — Arthur O'Dwyer | Written explanation via `std::function` and `std::any`; separates the three things the term is used for |
| [Breaking Dependencies: Type Erasure — A Design Analysis](https://www.youtube.com/watch?v=4eeESJQk-mw) — Klaus Iglberger, CppCon 2021 | Source of the comparison above, and of the decomposition into external polymorphism, bridge and prototype |
| [Breaking Dependencies: Type Erasure — The Implementation Details](https://www.youtube.com/watch?v=qn6OqefuH08) — Klaus Iglberger, CppCon 2022 ([slides](https://cppcon.digital-medium.co.uk/wp-content/uploads/2022/09/Type-Erasure-The-Implementation-Details-Klaus-Iglberger-CppCon-2022.pdf)) | Manual vtables, owning and non-owning wrappers, small buffer optimization |
| [Type-erased `UniquePrintable` and `PrintableRef`](https://quuxplusone.github.io/blog/2020/11/24/type-erased-printable/) — Arthur O'Dwyer | Owning and move-only versus non-owning and trivially copyable |
| [Type-erased `InplaceUniquePrintable`](https://quuxplusone.github.io/blog/2022/07/30/type-erased-inplace-printable/) — Arthur O'Dwyer | The variant that does not allocate |
| [ldionne/dyno](https://github.com/ldionne/dyno) | Dispatch mechanism as a policy rather than a fixed vtable |
| [folly::Poly](https://github.com/facebook/folly/blob/main/folly/docs/Poly.md) | Production implementation with composable interfaces |
| [Boost.TypeErasure](https://www.boost.org/doc/libs/release/doc/html/boost_typeerasure.html) | Fully general concept composition, and what it costs |
| [hgkjshegfskef/type-erasure-example](https://github.com/hgkjshegfskef/type-erasure-example) | Code accompanying the CppCon 2021 talk |
