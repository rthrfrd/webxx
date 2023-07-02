# `webxx`

> Declarative, composable, concise & fast HTML & CSS in C++.

[![linux](https://github.com/rthrfrd/webxx/actions/workflows/linux.yml/badge.svg)](https://github.com/rthrfrd/webxx/actions/workflows/linux.yml) [![codecov](https://codecov.io/gh/rthrfrd/webxx/branch/main/graph/badge.svg)](https://codecov.io/gh/rthrfrd/webxx)

```c++
#include "webxx.h"
using namespace Webxx;
std::string html = render(h1{"Hello ", i{"world!"}});
// <h1>Hello <i>world</i></h1>
```

## 🎛 Features

- No templating language: Use native C++ features for variables and composition.
- No external files or string interpolation: Your templates are compiled in.
- Component system provides easy modularity, re-use and automatic CSS scoping.
- Small, simple, single-file, header-only library.
- Compatible with C++17 and above (to support `std::string_view`), minimally stdlib dependent.

## 🏃 Getting started

### Installation

You can simply download and include [`include/webxx.h`](include/webxx.h) into your project, or clone this repository (e.g. using CMake [`FetchContent`](https://cmake.org/cmake/help/latest/module/FetchContent.html)).

#### Cmake integration

```cmake
# Define webxx as an interface library target:
add_library(webxx INTERFACE)
target_sources(webxx INTERFACE your/path/to/webxx.h)

# Link and include it in your target:
add_executable(yourApp main.cpp)
target_link_libraries(yourApp PRIVATE webxx)
target_include_directories(yourApp PRIVATE your/path/to/webxx)
```

### Demo

```c++
// main.cpp
#include <iostream>
#include <list>
#include <string>
#include "webxx.h"

int main () {
    using namespace Webxx;

    // Bring your own data model:
    bool is1MVisit = true;
    std::list<std::string> toDoItems {
        "Water plants",
        "Plug (memory) leaks",
        "Get back to that other project",
    };

    // Create modular components which include scoped CSS:
    struct ToDoItem : component<ToDoItem> {
        ToDoItem (const std::string &toDoText) : component<ToDoItem> {
            li { toDoText },
        } {}
    };

    struct ToDoList : component<ToDoList> {
        ToDoList (std::list<std::string> &&toDoItems) : component<ToDoList>{
            // Styles apply only to the component's elements:
            {
                {"ul",
                    // Hyphenated properties are camelCased:
                    listStyle{ "none" },
                },
            },
            // Component HTML:
            dv {
                h1 { "To-do:" },
                // Iterate over iterators:
                ul {
                    each<ToDoItem>(std::move(toDoItems))
                },
            },
        } {}
    };

    // Compose a full page:
    doc page {
        html {
            head {
                title { "Hello world!" },
                script { "alert('Howdy!');" },
                // Define global (unscoped) styles:
                style {
                    {"a",
                        textDecoration{"none"},
                    },
                },
                // Styles from components are gathered up here:
                styleTarget {},
            },
            body {
                // Set attributes:
                {_class{"dark", is1MVisit ? "party" : ""}},
                // Combine components, elements and text:
                ToDoList{std::move(toDoItems)},
                hr {},
                // Optionally include fragments:
                maybe(is1MVisit, [] () {
                    return fragment {
                        h1 {
                            "Congratulations you are the 1 millionth visitor!",
                        },
                        a { { _href{"/prize" } },
                            "Click here to claim your prize",
                        },
                    };
                }),
                "© Me 2022",
            },
        },
    };

    // Render to a string:
    std::cout << render(page) << std::endl;
}
```

## ⚠️ Beware - notes & gotchas

### Things `webxx` won't do

- __Parse:__ This library is just for constructing HTML & friends.
- __Validate:__ You can construct all the malformed HTML you like.
- __Escape:__ Strings are rendered raw - you must escape unsafe data to prevent XSS attacks.

### Quirks & inconsistencies

- The order in which component CSS styles are rendered cannot be relied upon due to the undefined order in which components may be initialised.
- Over 700 symbols are exposed in the `Webxx` namespace - use it considerately.
- Symbols are lowercased to mimic their typical appearance in HTML & CSS.
- HTML attributes are all prefixed with `_` (e.g. `href` -> `_href`).
- All `hy-phen-ated` tags, properties and attributes are translated to `camelCase` (e.g. `line-height` -> `lineHeight`).
- All CSS `@*` rules are renamed to `at*` (e.g. `@import` -> `atImport`).
- The following terms are specially aliased to avoid C++ keyword clashes:
  - __HTML Elements:__
    - `div` -> `dv`
    - `template` -> `template_`
  - __CSS Properties:__
    - `continue` -> `continue_`
    - `float` -> `float_`

## 🔥 Performance

Some basic [benchmarks](test/benchmark/benchmark.cpp) are built at `build/test/benchmark/webxx_benchmark` using [google-benchmark](https://github.com/google/benchmark.git). Webxx appears to be ~5-30x faster than using a template language like [inja](https://github.com/pantor/inja).

```sh
# gcc-13 on macOS Ventura:
Running build/test/benchmark/webxx_benchmark
--------------------------------------------------------------------
Benchmark                          Time             CPU   Iterations
--------------------------------------------------------------------
singleElementInja               9638 ns         9557 ns        71357
singleElementWebxx               341 ns          341 ns      1966635
singleElementSprintf             135 ns          135 ns      4925173
singleElementStringAppend       69.0 ns         68.9 ns      9895112
multiElementInja               11138 ns        11124 ns        59700
multiElementWebxx               1768 ns         1765 ns       367132
multiElementSprintf              199 ns          198 ns      3621183
multiElementStringAppend         365 ns          363 ns      1952036

# clang-14 on macOS Ventura:
Running build/test/benchmark/webxx_benchmark
--------------------------------------------------------------------
Benchmark                          Time             CPU   Iterations
--------------------------------------------------------------------
singleElementInja               7089 ns         7052 ns        97962
singleElementWebxx               266 ns          264 ns      2707292
singleElementSprintf            77.3 ns         77.1 ns      8468835
singleElementStringAppend       27.9 ns         27.9 ns     25437156
multiElementInja                9613 ns         9597 ns        67738
multiElementWebxx               1733 ns         1731 ns       385622
multiElementSprintf              182 ns          182 ns      3780494
multiElementStringAppend         221 ns          220 ns      2994178
```

## 🛠 Development

### Contributing

Contributions are super welcome, in the form of pull requests from Github forks. Please ensure you are able to make your contribution under the terms of the project license [(MIT)](LICENSE.md). New features may be rejected to limit scope creep.

### Roadmap / To-do

#### Now

- Reference documentation.

#### Sooner

- Add Mac & Windows builds.
- Avoid repeated hashing of component names when using hashed names.
- More benchmarking/testing (memory, libmxl2, more usage variations).

#### Later

- WASM usage (with two-way DOM binding).
- Indented render output.
- Publishing to package managers.

### Approach

- Simplicity, safety and accessibility are prioritised.
- Idiomatic C++ is used wherever possible.
- Scope creep is treated with caution.

### Orientation

The library is sectioned into several modules:

- __CSS:__ Classes for constructing CSS stylesheets.
- __HTML:__ Classes for construcing HTML elements & documents.
- __Component:__ Abstration for a modular combination of CSS & HTML.
- __Rendering:__ Functions for rendering constructed Components, HTML & CSS into strings.
- __Utility:__ Helper functions for dynamically generating content.
- __Public:__ The interface users of this library can consume.
