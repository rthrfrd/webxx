#include "doctest/doctest.h"
#include "fmt/core.h"
#include "webxx.h"

TEST_SUITE("Component") {
    using namespace Webxx;

    TEST_CASE("Component can be created") {
        component myCom {
            "myCom",
            {
                {".title",
                    color{"green"},
                },
                {".summary",
                    color{"blue"},
                },
            },
            dv{
                h1{{_class{"title"}}, "Hello"},
                p{{_class{"summary"}}, "World."},
            },
        };
        std::string expectedHashAttr = std::string("data-c").append(internal::hash<internal::hashLen>("myCom"));

        CHECK(myCom.css->size() == 2);

        SUBCASE("Component can be rendered") {
            CHECK(render(myCom) == "<div data-cmyCom><h1 class=\"title\" data-cmyCom>Hello</h1><p class=\"summary\" data-cmyCom>World.</p></div>");
        }

        SUBCASE("Component styles are collected and rendered") {
            html myPage {
                head {
                    styleTarget{},
                },
                body {
                    myCom,
                    myCom,
                },
            };

            CHECK(render(myPage) ==
            "<html>"
                "<head>"
                    // Collected style only appears once:
                    "<style>[data-cmyCom].title{color:green;}[data-cmyCom].summary{color:blue;}</style>"
                "</head>"
                "<body>"
                    // HTML appears twice:
                    "<div data-cmyCom><h1 class=\"title\" data-cmyCom>Hello</h1><p class=\"summary\" data-cmyCom>World.</p></div>"
                    "<div data-cmyCom><h1 class=\"title\" data-cmyCom>Hello</h1><p class=\"summary\" data-cmyCom>World.</p></div>"
                "</body>"
            "</html>"
            );
        }

        SUBCASE("Component can be rendered with hash") {
            CHECK(render(myCom, {true}) == fmt::format(
                "<div {0}>"
                    "<h1 class=\"title\" {0}>Hello</h1>"
                    "<p class=\"summary\" {0}>World.</p>"
                "</div>",
                expectedHashAttr
            ));
        }

        SUBCASE("Component styles are collected and rendered with hash") {
            dv root {{
                styleTarget{},
                myCom,
            }};

            CHECK(render(root, {true}) == fmt::format(
                "<div>"
                    "<style>[{0}].title{{color:green;}}[{0}].summary{{color:blue;}}</style>"
                    "<div {0}><h1 class=\"title\" {0}>Hello</h1><p class=\"summary\" {0}>World.</p></div>"
                "</div>",
                expectedHashAttr
            ));
        }

        SUBCASE("Only component styles can be rendered") {
            html myPage {
                head {
                    styleTarget{},
                    // Non-component styles are not included:
                    style{{"a", color{"red"}}},
                },
                body {
                    myCom,
                    myCom,
                },
            };

            CHECK(renderCss(myPage) == "[data-cmyCom].title{color:green;}[data-cmyCom].summary{color:blue;}");
        }

        SUBCASE("Only component styles can be rendered, hashed") {
            html myPage {
                head {
                    styleTarget{},
                    // Non-component styles are not included:
                    style{{"a", color{"red"}}},
                },
                body {
                    myCom,
                    myCom,
                },
            };

            CHECK(renderCss(myPage, {true}) == fmt::format(
                "[{0}].title{{color:green;}}"
                "[{0}].summary{{color:blue;}}",
                expectedHashAttr
            ));
        }
    }

    TEST_CASE("Component can be created with shared static styles") {
        static const styles styles {
            {"h1",
                color{"red"},
            },
        };

        component comA {
            "comA",
            styles,
            dv{
                h1{"A"},
            },
        };

        component comB {
            "comB",
            styles,
            dv{
                h1{"B"},
                comA,
            },
        };

        CHECK(comA.css.get() == comB.css.get());

        SUBCASE("Component with shared static styles can be rendered") {
            CHECK(render(comB) == "<div data-ccomB><h1 data-ccomB>B</h1><div data-ccomA><h1 data-ccomA>A</h1></div></div>");
        }

        SUBCASE("Component with shared static styles are collected and rendered") {
            html myPage {
                head {
                    styleTarget{},
                },
                body {
                    comB,
                },
            };

            std::string html = render(myPage);

            std::string htmlStart{
                "<html>"
                    "<head>"
                        "<style>"
            };
            // Collected styles from all components, with each only appearing once:
            std::string cssComA{"[data-ccomA]h1{color:red;}"};
            std::string cssComB{"[data-ccomB]h1{color:red;}"};
            std::string htmlEnd{
                        "</style>"
                    "</head>"
                    "<body>"
                        "<div data-ccomB><h1 data-ccomB>B</h1><div data-ccomA><h1 data-ccomA>A</h1></div></div>"
                    "</body>"
                "</html>"
            };

            CHECK(html.find(htmlStart) == 0);
            CHECK(html.find(cssComA) > 0);
            CHECK(html.find(cssComB) > 0);
            CHECK(html.rfind(htmlEnd) > html.find(cssComA));
            CHECK(html.rfind(htmlEnd) > html.find(cssComB));
        }
    }

    TEST_CASE("Components can be nested") {
        component comA {
            "comA",
            {
                {".a", color{"green"}},
            },
            dv{{_class{"a"}},
                "Hello A",
            },
        };
        component comB {
            "comB",
            {
                {".b", color{"blue"}},
            },
            dv{{_class{"b"}},
                comA,
                "Hello B",
            },
        };
        doc doc {
            html {
                head {
                    styleTarget{},
                },
                body {
                    comB,
                },
            },
        };

        std::string html = render(doc);

        std::string htmlStart{
            "<!doctype html>"
            "<html>"
                "<head>"
                    "<style>"
        };
        // Collected styles from all components, with each only appearing once:
        std::string cssComA{"[data-ccomA].a{color:green;}"};
        std::string cssComB{"[data-ccomB].b{color:blue;}"};
        std::string htmlEnd{
                "</style>"
                "</head>"
                "<body>"
                    "<div class=\"b\" data-ccomB>"
                        "<div class=\"a\" data-ccomA>"
                            "Hello A"
                        "</div>"
                        "Hello B"
                    "</div>"
                "</body>"
            "</html>"
        };

        CHECK(html.find(htmlStart) == 0);
        CHECK(html.find(cssComA) > 0);
        CHECK(html.find(cssComB) > 0);
        CHECK(html.rfind(htmlEnd) > html.find(cssComA));
        CHECK(html.rfind(htmlEnd) > html.find(cssComB));
    }
}
