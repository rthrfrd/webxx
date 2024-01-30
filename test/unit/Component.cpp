#include "doctest/doctest.h"
#include "fmt/core.h"
#include "webxx.h"

TEST_SUITE("Component") {
    using namespace Webxx;

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
                {
                    link{{_rel{"test"}}},
                },
            } {}
        };

        MyCom myCom{};
        auto myComId = myCom.id.begin();

        SUBCASE("Component can be rendered") {
            CHECK(render(myCom) == fmt::format("<div lang=\"x-{0}\"><h1 class=\"title\">Hello</h1><p class=\"summary\">World.</p></div>", myComId));
        }
        SUBCASE("Component styles are collected and rendered") {
            html myPage {
                head {
                    styleTarget{},
                },
                body {
                    MyCom{},
                    MyCom{},
                },
            };

            CHECK(render(myPage) == fmt::format(
            "<html>"
                    "<head>"
                        // Collected style only appears once:
                        "<style>.title:lang(x-{0}){{color:green;}}.summary:lang(x-{0}){{color:blue;}}</style>"
                    "</head>"
                    "<body>"
                        // HTML appears twice:
                        "<div lang=\"x-{0}\"><h1 class=\"title\">Hello</h1><p class=\"summary\">World.</p></div>"
                        "<div lang=\"x-{0}\"><h1 class=\"title\">Hello</h1><p class=\"summary\">World.</p></div>"
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
                    MyCom{},
                    MyCom{},
                },
            };

            CHECK(renderCss(myPage) == fmt::format(".title:lang(x-{0}){{color:green;}}.summary:lang(x-{0}){{color:blue;}}", myComId));
        }

        SUBCASE("Component head elements are collected and rendered") {
            html myPage {
                head {
                    headTarget{},
                },
                body {
                    MyCom{},
                    MyCom{},
                },
            };

            CHECK(render(myPage) == fmt::format(
            "<html>"
                    "<head>"
                        // Collected elements only appears once:
                        "<link rel=\"test\"/>"
                    "</head>"
                    "<body>"
                        // HTML appears twice:
                        "<div lang=\"x-{0}\"><h1 class=\"title\">Hello</h1><p class=\"summary\">World.</p></div>"
                        "<div lang=\"x-{0}\"><h1 class=\"title\">Hello</h1><p class=\"summary\">World.</p></div>"
                    "</body>"
                "</html>",
                myComId
            ));
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
            ComB(ComA&& comA) : component<ComB> {
                {
                    {".b", color{"blue"}},
                },
                dv{{_class{"b"}},
                    std::move(comA),
                    "Hello B",
                },
            } {}
        };

        ComA comA{ComA{"Hello A"}};
        ComB comB{std::move(comA)};

        doc doc {
            html {
                head {
                    styleTarget{},
                },
                body {
                    std::move(comB),
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
        std::string cssComA{fmt::format(".a:lang(x-{0}){{color:green;}}", comA.id.begin())};
        std::string cssComB{fmt::format(".b:lang(x-{0}){{color:blue;}}", comB.id.begin())};
        std::string htmlEnd{fmt::format(
                "</style>"
                "</head>"
                "<body>"
                    "<div c{1} class=\"b\">"
                        "<div c{0} class=\"a\">"
                            "Hello A"
                        "</div>"
                        "Hello B"
                    "</div>"
                "</body>"
            "</html>",
            comA.id.begin(),
            comB.id.begin()
        )};

        CHECK(html.find(htmlStart) == 0);
        CHECK(html.find(cssComA) > 0);
        CHECK(html.find(cssComB) > 0);
        CHECK(html.rfind(htmlEnd) > html.find(cssComA));
        CHECK(html.rfind(htmlEnd) > html.find(cssComB));
    }
}
