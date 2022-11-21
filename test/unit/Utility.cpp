#include "doctest/doctest.h"
#include "webxx.h"

TEST_SUITE("Utility") {
    using namespace Webxx;

    struct Post { std::string id, title, summary, url; };
    std::vector<Post> posts{
        {"...", "10 ways to leak memory", "12 of them you already know", "http://..."},
        {"...", "1 simple trick", "Your compiler hates this", "http://..."},
        {"...", "Is C++ dead? 💀", "If you have to ask...", "http://..."}
    };
    std::vector<Post> noPosts{};

    li postItem (const Post &post) {
        return {post.title};
    }

    TEST_CASE("Maybe include HTML") {
        std::string will{"You will see me."};
        std::string wont{"You won't see me."};


        SUBCASE("Maybe include HTML without forwarding") {
            dv myDiv{
                maybe(false, [&wont] () { return wont; }),
                maybe(true, [&will] () { return will; }),
            };

            CHECK(render(myDiv) == "<div>You will see me.</div>");
        }

        SUBCASE("Maybe include HTML with forwarding") {
            dv myDiv{
                maybe(false, wont, [] (std::string &x) { return x; }),
                maybe(true, will, [] (std::string &x) { return x; }),
            };

            CHECK(render(myDiv) == "<div>You will see me.</div>");
        }
    }

    TEST_CASE("Maybe on bool operator") {
        struct MaybeAThing {
            std::string thingIAm;

            operator bool() {
                return !thingIAm.empty();
            }
        };

        MaybeAThing notAThing {};
        MaybeAThing isAThing {"Inflatible trousers"};

        SUBCASE("Maybe on a bool operator without forwarding") {
            dv myDiv{
                maybe(isAThing, [&isAThing] () {return p{isAThing.thingIAm}; }),
                maybe(notAThing, [] () { return p{"Not a thing."}; }),
            };

            CHECK(render(myDiv) == "<div><p>Inflatible trousers</p></div>");
        }

        SUBCASE("Maybe on a bool operator with forwarding") {
            dv myDiv{
                maybe(isAThing, isAThing, [] (MaybeAThing &x) {return p{x.thingIAm}; }),
                maybe(notAThing, notAThing, [] (MaybeAThing &x) { return p{"Not a thing.", x.thingIAm}; }),
            };

            CHECK(render(myDiv) == "<div><p>Inflatible trousers</p></div>");
        }
    }

    TEST_CASE("Each with lambda") {
        ol myList {
            each(posts, [] (const Post &post) { return li {post.title}; }),
        };

        CHECK(render(myList) ==
            "<ol>"
                "<li>10 ways to leak memory</li>"
                "<li>1 simple trick</li>"
                "<li>Is C++ dead? 💀</li>"
            "</ol>"
        );
    }

    TEST_CASE("Each empty with lambda") {
        ol myList {
            each(noPosts, [] (const Post &post) { return li {post.title}; }),
        };
        CHECK(render(myList) =="<ol></ol>");
    }

    TEST_CASE("Each with function") {
        ol myList {
            each(posts, postItem),
        };

        CHECK(render(myList) ==
            "<ol>"
                "<li>10 ways to leak memory</li>"
                "<li>1 simple trick</li>"
                "<li>Is C++ dead? 💀</li>"
            "</ol>"
        );
    }
}