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

- The order in which CSS styles belonging to different components are rendered cannot be relied upon, due to the undefined order in which components may be initialised.
- Over 700 symbols are exposed in the `Webxx` namespace - use it considerately.
- Symbols are lowercased to mimic their typical appearance in HTML & CSS.
- HTML attributes are all prefixed with `_` (e.g. `href` -> `_href`).
- All `kebab-case` tags, properties and attributes are translated to `camelCase` (e.g. `line-height` -> `lineHeight`).
- All CSS `@*` rules are renamed to `at*` (e.g. `@import` -> `atImport`).
- The following terms are specially aliased to avoid C++ keyword clashes:
  - __HTML Elements:__
    - `div` -> `dv`
    - `template` -> `template_`
  - __CSS Properties:__
    - `continue` -> `continue_`
    - `float` -> `float_`

### Memory safety & efficiency

- The type of value you provide as content to webxx elements/attributes/etc generally determines whether webxx will make a copy of the value or not:
  - __Will not result in a copy:__
    - `[const] char*`
    - `[const] std::string_view`
    - `[const] std::string&&`
  - __Will result in a copy:__
    - `[const] std::string`
- As it is possible to render elements at a different time from constructing them, __you must make sure that the objects you reference in your document have not been destroyed before you render__.
- It is encouraged to use `std::move` to move variables into the components where they are needed, both for performance and to ensure their lifetimes are extended to that of the webxx document.
- Alternatively you can pass in variables by value, so that the document retains its own copy of the data it needs to render, which cannot fall out of scope.
- Additional care must be taken when providing `std::string_view`s to the document. While performant, you must ensure the underlying string has not been destroyed.

## 📖 User guide

### 1. Components (a.k.a scope, reusability & composition)

A component is any C++ struct/class that inherits from `Webxx::component`. It is made
up of HTML, along with optional parameters & CSS styles. The CSS is "scoped": Any CSS styles defined in a component apply only to the HTML elements that belong to that component:

```c++
using namespace Webxx;

// Components can work with whatever data model you want:
struct TodoItemData {
    std::string description;
    bool isCompleted;
};

struct TodoItem : component<TodoItem> {
    // Paramters are defined in the constructor:
    TodoItem (TodoItemData &&todoItem) : component<TodoItem> {
        // CSS (optional, can be omitted):
        {
            {"li.completed",
                textDecoration{"line-through"},
            },
        },
        // HTML:
        li {
            // Element attributes appear first...
            {_class{todoItem.isCompleted ? "completed" : ""}},
            // ...followed by content:
            todoItem.description,
        },
        // 'Head elements' - to add to <head> (optional, can be omitted):
        {
            // Useful for e.g. preloading assets that this component might use:
            link{{_rel{"preload"}, _href{"/background.gif"}, _as{"image"}, _type{"image/gif"}}},
        }
    } {}
};
```

It is encouraged to move variables into the components where they are needed, to avoid any risk of them falling out of scope:

```c++
TodoItem generateTodoItem () {
    TodoItemData item{"Thing to do!", false};
    // If we did not use std::move, description would fall
    // out of scope and be destroyed before being rendered:
    return TodoItem{std::move(item)};
}

auto todoItem = generateTodoItem();
auto html = render(todoItem); // <li>Thing to do!</li>
```

It is straightforwards to repeat components using the `each` helper function, or optionally include them using `maybe`:

```c++
struct TodoList : component<TodoList> {
    TodoList (std::list<TodoItemData> &&todoItems) : component<TodoList> {
        ul {
            // Show each item in the list:
            each<TodoItem>(std::move(todoItems)),
            // Show a message if the list is empty:
            maybe(todoItems.empty(), [] () {
                return li{"You're all done!"};
            }),
        },
    } {}
};
```

Components and other nodes can be composed arbitrarily. For example this allows you to create structural components with slots into which other components can be inserted:

```c++
struct TodoPage : component<TodoPage> {
    TodoPage (node &&titleEl, node &&mainEl) : component<TodoPage> {
        doc { // Creates the <doctype>
            html{ // Creates the <html>
                head {
                    title{"Todo"},
                    // Special element to collect all component CSS:
                    styleTarget{},
                    // Special element to collect all component head elements:
                    headTarget{},
                },
                body{
                    std::move(titleEl),
                    main {
                        std::move(mainEl),
                    }
                },
            },
        },
    } {}
};

auto pageHtml = render(TodoPage{
    h1{"My todo list"},
    TodoList{{
        {"Clean the car", false},
        {"Clean the dog", false},
        {"Clean the browser history", true},
    }},
});
```

__The `styleTarget` element must appear somewhere in the HTML, in order for the CSS defined in each component to work. Likewise for the `headTarget` and component 'head elements'.__

### 2. Loops, Conditionals & Fragments

The `each` function can be used to generate elements, and supports two approaches that can produce equivalent outputs:

```c++
std::vector<std::string> letters{"a", "b", "c"};

// Using a lambda (or other callable) allows arbitrary complexity:
fragment byLambda = each(letters, [] (std::string letter) {
    return li { letter };
});

// Using the template approach is best for concise simplicity:
fragment byTemplate = each<li>(letters);

auto isSame = render(byLambda) == render(byTemplate); // is true
```

The `loop` function behaves the same as `each`, but additionally provides a second parameter to the callback with information about the loop:

```c++
loop(letters, [] (const std::string& letter, const Loop& loop) {
    return li { std::to_string(loop.index), ": ", letter };
});
```

A `fragment` contains all the generated elements for each item. A `fragment` is an "invisible" element; it will not show up in the rendered output (but its children will).

They can be used to pass around multiple elements without wrapping them in a containing `div` or similar. For example they let you produce multiple elements for each item in a loop:

```c++
auto html = render(each(letters, [] (std::string letter) {
    return fragment {
        p{letter},
        hr{},
    };
}));
// html = "<p>a</p><hr/><p>b</p><hr/><p>c</p><hr/>"
```

### 3. Placeholders (a.k.a how to i18n)

Placeholders enable you to perform post-processing of the document at render time. This can be useful for tasks such as internationalization.

You can define a "populator" function, which is called for every placeholder that is encountered while rendering the document.

```c++
std::unordered_map<std::string_view,std::string_view> translations {
    {"Hello", "Hej"},
    {"world", "värld"},
};

h1 title {_{"Hello"}, _{"world"}, "!"};

auto translatedHtml = render(title, {
    false,
    [&translations] (
        const std::string_view key,
        const std::string_view
    ) -> const std::string_view {
        return translations.at(key);
    }
});

// translatedHtml = "<h1>Hey värld!</h1>"
```

### 4. Custom elements & attributes

You can define your own elements and attributes in the same way that webxx does internally:

```c++
constexpr static char customElTag[] = "custom-el";
using customEl = Webxx::el<customElTag>;

constexpr static char customDataThingAttr[] = "data-thing";
using dataThing = Webxx::attr<customDataThingAttr>;

render(customElTag{
    {
        dataThing{"value"},
    },
    "Hi",
}); // <custom-el data-thing="value">Hi</custom-el>
```

### 5. Rendering

By default the `render` function appends everything to an internal string buffer, which it returns. However if you want to get that first byte out before rendering the whole doc, you can hook in with a function to stream the output while the rendering is still in progress:

```c++
#include <sstream>
std::stringstream out;
size_t chunkSize{256};

// Our hook function takes rendered data, and a reference to the internal buffer:
void onRenderData (const std::string_view data, std::string &buffer) {
    // In this case we are going to still use the internal buffer...
    buffer.append(data);
    if (buffer.size() >= chunkSize) {
        // ...and then stream it out every 256 characters:
        out << buffer;
        buffer.clear();
    }
}

doc myDoc {
    head {},
    body {"Hello world"},
};

// Start the render:
std::string leftovers = render(myDoc, {
    nullptr, // We're not using a placeholder populator.
    onRenderData, // Provide our render output receiver.
    chunkSize // Preset the size of the internal buffer.
});

// Some data might be left in the buffer - this is returned so we can stream that too:
out << leftovers;
```

You can also defer work until calling `render` by using `lazy`. However lazy blocks are still executed in a pass _before_ the first bytes are rendered.

```c++
std::string text{"Hello"};

dv myDiv{
    // Evaluated now:
    h1{std::string{text}},
    // Lazy block takes a function:
    lazy{[&text] () {
        // Only evaluated after render() is called below:
        return p{text};
    }},
};

text = "world";

render(myDiv); // <div><h1>Hello</h1><p>world</p></div>
```

## 🔥 Performance

Some basic [benchmarks](test/benchmark/benchmark.cpp) are built at `build/test/benchmark/webxx_benchmark` using [google-benchmark](https://github.com/google/benchmark.git). Webxx appears to be ~5-30x faster than using a template language like [inja](https://github.com/pantor/inja).

```sh
# clang-14 on macOS Ventura:
Running build/test/benchmark/webxx_benchmark
--------------------------------------------------------------------
Benchmark                          Time             CPU   Iterations
--------------------------------------------------------------------
singleElementInja               6218 ns         6213 ns        96863
singleElementWebxx               147 ns          147 ns      4570682
singleElementSprintf            72.0 ns         71.9 ns      9102612
singleElementStringAppend       25.9 ns         25.9 ns     26220567
multiElementInja                9384 ns         9379 ns        69070
multiElementWebxx                430 ns          430 ns      1646702
multiElementSprintf              176 ns          176 ns      3792640
multiElementStringAppend         203 ns          203 ns      3229825
loop1kInja                   1410894 ns      1410387 ns          468
loop1kWebxx                   690407 ns       689945 ns          861
loop1kStringAppend            110683 ns       110615 ns         5883

# gcc-13 on macOS Ventura:
Running build/test/benchmark/webxx_benchmark
--------------------------------------------------------------------
Benchmark                          Time             CPU   Iterations
--------------------------------------------------------------------
singleElementInja               6804 ns         6787 ns        95385
singleElementWebxx               240 ns          239 ns      2579599
singleElementSprintf            74.8 ns         74.7 ns      8870416
singleElementStringAppend       27.6 ns         27.5 ns     25333039
multiElementInja                9630 ns         9616 ns        65712
multiElementWebxx               1015 ns         1013 ns       622698
multiElementSprintf              177 ns          177 ns      3706096
multiElementStringAppend         211 ns          210 ns      3207111
loop1kInja                   1537252 ns      1519808 ns          426
loop1kWebxx                   927711 ns       926574 ns          659
loop1kStringAppend            113185 ns       113055 ns         5656
```

## 🛠 Development

### Contributing

Contributions are super welcome, in the form of pull requests from Github forks. Please ensure you are able to make your contribution under the terms of the project license [(MIT)](LICENSE.md). New features may be rejected to limit scope creep.

### Roadmap / To-do

- Now
    - Optimisations to avoid heap allocations where sizes may be known.
    - Add Mac & Windows builds.
    - More benchmarking/testing (memory, libmxl2, more usage variations).
- Later
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
- __HTML:__ Classes for constructing HTML elements & documents.
- __Component:__ Abstraction for a modular combination of CSS & HTML.
- __Rendering:__ Functions for rendering constructed Components, HTML & CSS into strings.
- __Utility:__ Helper functions for dynamically generating content.
- __Public:__ The interface users of this library can consume.

### Useful commands

```sh
# Open in Xcode:
cmake -G Xcode -B build-xcode
```