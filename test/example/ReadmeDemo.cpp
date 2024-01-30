#include <iostream>
#include <list>
#include "doctest/doctest.h"
#include "webxx.h"

TEST_SUITE("README") {
    using namespace Webxx;

    TEST_CASE("Demo") {
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
                    _class{"dark", is1MVisit ? "party" : ""},
                    // Combine components, elements and text:
                    ToDoList{std::move(toDoItems)},
                    hr {},
                    // Optionally include fragments:
                    maybe(is1MVisit, [] () {
                        return fragment {
                            h1 {
                                "Congratulations you are the 1 millionth visitor!",
                            },
                            a { _href{"/prize" },
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
}