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

        CHECK(myCom.css.size() == 2);

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
                    "<style>.title[data-cmyCom]{color:green;}.summary[data-cmyCom]{color:blue;}</style>"
                "</head>"
                "<body>"
                    // HTML appears twice:
                    "<div data-cmyCom><h1 class=\"title\" data-cmyCom>Hello</h1><p class=\"summary\" data-cmyCom>World.</p></div>"
                    "<div data-cmyCom><h1 class=\"title\" data-cmyCom>Hello</h1><p class=\"summary\" data-cmyCom>World.</p></div>"
                "</body>"
            "</html>"
            );
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

            CHECK(renderCss(myPage) == ".title[data-cmyCom]{color:green;}.summary[data-cmyCom]{color:blue;}");
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
        std::string cssComA{".a[data-ccomA]{color:green;}"};
        std::string cssComB{".b[data-ccomB]{color:blue;}"};
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
