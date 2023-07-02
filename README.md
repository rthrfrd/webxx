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
- Compatible with C++11 and above, minimally stdlib dependent.

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
#include <string>
#include <vector>
#include "webxx.h"

int main () {
    using namespace Webxx;

    // Bring your own data model:
    bool is1MVisit = true;

    // Create modular components which include scoped CSS:
    struct ToDoList : component<ToDoList> {
        ToDoList (std::vector<std::string> &&toDoItems) : component<ToDoList>{
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
                ul { each(toDoItems, [] (const std::string &item) {
                    return li { item };
                }) },
            },
        } {}
    };

    ToDoList toDoList{{
        "Water plants",
        "Plug (memory) leaks",
        "Get back to that other project",
    }};

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
                toDoList,
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
- All `hy-phen-ated` tags, properties and attributes are translated to `camelCase`.
- All CSS `@*` rules are renamed to `at*` (e.g. `@import` -> `atImport`).
- The following terms are aliased to avoid C++ keyword clashes:
  - __HTML Elements:__
    - `div` -> `dv`
    - `template` -> `template_`
  - __CSS Properties:__
    - `continue` -> `continue_`
    - `float` -> `float_`

## 🛠 Development

### Contributing

Contributions are super welcome, in the form of pull requests from Github forks. Please ensure you are able to make your contribution under the terms of the project license [(MIT)](LICENSE.md). New features may be rejected to limit scope creep.

### Roadmap / To-do

#### Now

- Reference documentation.

#### Sooner

- Add Mac & Windows builds.
- Avoid repeated hashing of component names when using hashed names.
- More benchmarking/testing (memory, libmxl2, variations).

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
