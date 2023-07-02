#include "doctest/doctest.h"
#include "fmt/core.h"
#include "webxx.h"

TEST_SUITE("Component") {
    using namespace Webxx;

    template <class T>
    std:: string comId(T &&obj) {
        return std::to_string(typeid(obj).hash_code());
    }

    TEST_CASE("Component can be created") {
        struct MyCom : component<MyCom> {
            MyCom() : component<MyCom> {
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

            } {}
        };

        MyCom myCom{};
        auto myComId = comId(myCom);

        CHECK(myCom.css.size() == 2);

        SUBCASE("Component can be rendered") {
            CHECK(render(myCom) == fmt::format("<div data-c{0}><h1 class=\"title\" data-c{0}>Hello</h1><p class=\"summary\" data-c{0}>World.</p></div>", myComId));
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

            CHECK(render(myPage) == fmt::format(
            "<html>"
                    "<head>"
                        // Collected style only appears once:
                        "<style>.title[data-c{0}]{{color:green;}}.summary[data-c{0}]{{color:blue;}}</style>"
                    "</head>"
                    "<body>"
                        // HTML appears twice:
                        "<div data-c{0}><h1 class=\"title\" data-c{0}>Hello</h1><p class=\"summary\" data-c{0}>World.</p></div>"
                        "<div data-c{0}><h1 class=\"title\" data-c{0}>Hello</h1><p class=\"summary\" data-c{0}>World.</p></div>"
                    "</body>"
                "</html>",
                myComId
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

            CHECK(renderCss(myPage) == fmt::format(".title[data-c{0}]{{color:green;}}.summary[data-c{0}]{{color:blue;}}", myComId));
        }
    }

    TEST_CASE("Components can be nested") {
        struct ComA : component<ComA> {
            ComA(std::string msg) : component<ComA> {
                {
                    {".a", color{"green"}},
                },
                dv{{_class{"a"}},
                    msg,
                },
            } {}
        };

        struct ComB : component<ComB> {
            ComB(const ComA &comA) : component<ComB> {
                {
                    {".b", color{"blue"}},
                },
                dv{{_class{"b"}},
                    comA,
                    "Hello B",
                },
            } {}
        };

        ComA comA{ComA{"Hello A"}};
        ComB comB{comA};

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
        std::string cssComA{fmt::format(".a[data-c{0}]{{color:green;}}", comId(comA))};
        std::string cssComB{fmt::format(".b[data-c{0}]{{color:blue;}}", comId(comB))};
        std::string htmlEnd{fmt::format(
                "</style>"
                "</head>"
                "<body>"
                    "<div class=\"b\" data-c{1}>"
                        "<div class=\"a\" data-c{0}>"
                            "Hello A"
                        "</div>"
                        "Hello B"
                    "</div>"
                "</body>"
            "</html>",
            comId(comA),
            comId(comB)
        )};

        CHECK(html.find(htmlStart) == 0);
        CHECK(html.find(cssComA) > 0);
        CHECK(html.find(cssComB) > 0);
        CHECK(html.rfind(htmlEnd) > html.find(cssComA));
        CHECK(html.rfind(htmlEnd) > html.find(cssComB));
    }
}
