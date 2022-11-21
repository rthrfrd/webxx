#include "doctest/doctest.h"
#include "webxx.h"

TEST_SUITE("Variations") {
    using namespace Webxx;

    constexpr static char goal[] = "<!doctype html>"
    "<html>"
        "<head>"
            "<title>Hello title!</title>"
            "<link rel=\"stylesheet\" href=\"./styles.css\"/>"
            "<style>.a{font-weight:bold;}.b{opacity:0.5;}</style>"
        "</head>"
        "<body>"
            "<nav>"

            "</nav>"
        "</body>"
    "</html>"
    ;

    TEST_CASE("Variation - literal") {
        doc doc{
            html{
                head{
                    title{"Hello title!"},
                    link{{_rel{"stylesheet"}, _href{"./styles.css"}}},
                    style{{".a", fontWeight{"bold"}}, {".b", opacity("0.5")}},
                },
                body{
                    nav{

                    },
                },
            },
        };

        CHECK(render(doc) == std::string(goal));
    }
}