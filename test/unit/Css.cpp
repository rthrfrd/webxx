#include "doctest/doctest.h"
#include "webxx.h"

TEST_SUITE("CSS Stylesheet") {
    using namespace Webxx;

    static constexpr char webkitSomethingName[] = "-webkit-something";
    using webkitSomething = property<webkitSomethingName>;

    TEST_CASE("Property can have value") {
        color property{"red"};

        CHECK(std::string(property.label) == "color");
        CHECK(property.valueOwned == "red");

        SUBCASE("Property with value can be rendered") {
            CHECK(render(property) == "color:red;");
        }
    }

    TEST_CASE("Custom property can be defined") {
        webkitSomething property{"foo"};

        CHECK(property.label == webkitSomethingName);
        CHECK(property.valueOwned == "foo");

        SUBCASE("Customer property can be rendered") {
            CHECK(render(property) == "-webkit-something:foo;");
        }
    }

    TEST_CASE("Rule can have no properties") {
        rule rule{"body p.selector"};

        SUBCASE("Rule with no properties can be rendered") {
            CHECK(render(rule) == "body p.selector{}");
        }
    }

    TEST_CASE("Rule can have no properties and multiple selectors") {
        rule rule{{".a", ".b"}};

        SUBCASE("Rule with no properties and multiple selectors can be rendered") {
            CHECK(render(rule) == ".a,.b{}");
        }
    }

    TEST_CASE("Rule can have single property") {
        rule rule{".selector",
            color{"red"},
        };

        CHECK(rule.children.size() == 1);

        SUBCASE("Rule with single property can be rendered") {
            CHECK(render(rule) == ".selector{color:red;}");
        }
    }

    TEST_CASE("Rule can have single property and multiple selectors") {
        rule rule{{".a", ".b"},
            color{"red"},
        };

        CHECK(rule.children.size() == 1);

        SUBCASE("Rule with single property and multiple selectors can be rendered") {
            CHECK(render(rule) == ".a,.b{color:red;}");
        }
    }

    TEST_CASE("Rule can have multiple properties") {
        rule rule{".selector",
            color{"red"},
            backgroundColor{"#000"},
            font{"128px \"Comic Sans\""}
        };

        CHECK(rule.children.size() == 3);

        SUBCASE("Rule with multiple properties can be rendered") {
            CHECK(render(rule) == ".selector{color:red;background-color:#000;font:128px \"Comic Sans\";}");
        }
    }

    TEST_CASE("Rule can have multiple properties and multiple selectors") {
        rule rule{{".a", ".b"},
            color{"red"},
            backgroundColor{"#000"},
            font{"128px \"Comic Sans\""}
        };

        CHECK(rule.children.size() == 3);

        SUBCASE("Rule with multiple properties can be rendered") {
            CHECK(render(rule) == ".a,.b{color:red;background-color:#000;font:128px \"Comic Sans\";}");
        }
    }

    TEST_CASE("Single @rule can have value") {
        atImport atRule{"url(/some/other.css)"};


        SUBCASE("Single @rule with value can be rendered") {
            CHECK(render(atRule) == "@import url(/some/other.css);");
        }
    }

    TEST_CASE("Nested @rule can have styles") {
        atMedia atRule {"screen and (min-width: 900px)", {
            {"body", color{"red"}},
            {"p", color{"blue"}},
        }};

        SUBCASE("Nested @rule with styles can be rendered") {
            CHECK(render(atRule) == "@media screen and (min-width: 900px){body{color:red;}p{color:blue;}}");
        }
    }

    TEST_CASE("Nested @rule can be nested") {
        atMedia atRule {"screen and (min-width: 900px)", {
            atSupports {"(display: flex)", {
                {"body", color{"red"}},
                {"p", color{"blue"}},
            }},
        }};

        SUBCASE("Nested @rules can be rendered") {
            CHECK(render(atRule) == "@media screen and (min-width: 900px){@supports (display: flex){body{color:red;}p{color:blue;}}}");
        }
    }

    TEST_CASE("Sheet can be empty") {
        styles sheet;

        SUBCASE("Empty sheet can be rendered") {
            CHECK(render(sheet) == "");
        }
    }

    TEST_CASE("Sheet can have single rule") {
        styles sheet{
            {".selector",
                color{"red"},
            },
        };

        CHECK(sheet.size() == 1);

        SUBCASE("Sheet with single rule can be rendered") {
            CHECK(render(sheet) == ".selector{color:red;}");
        }
    }

    TEST_CASE("Sheet can have multiple rules") {
        styles sheet{
            {".a",
                color{"red"},
            },
            {".b",
                color{"green"},
            },
            {".c",
                color{"blue"},
            },
        };

        CHECK(sheet.size() == 3);

        SUBCASE("Sheet with multiple rules can be rendered") {
            CHECK(render(sheet) == ".a{color:red;}.b{color:green;}.c{color:blue;}");
        }
    }
}
