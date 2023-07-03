#include "doctest/doctest.h"
#include "webxx.h"

TEST_SUITE("Attribute") {
    using namespace Webxx;

    constexpr static char customDataAttr[] = "data-custom";
    using _dataCustom = attr<customDataAttr>;

    TEST_CASE("Attribute can be created empty") {
        _class attribute{};

        CHECK(std::string(attribute.name) == "class");
        CHECK(attribute.values.empty());

        SUBCASE("Empty attribute can be rendered") {
            std::string rendered = render(attribute);
            CHECK(rendered == "class");
        }
    }

    TEST_CASE("Attribute can be created with string literal") {
        _class attribute{"big"};

        CHECK(std::string(attribute.name) == "class");
        CHECK(attribute.values[0].valueOwned == "big");

        SUBCASE("String literal attribute can be rendered") {
            std::string rendered = render(attribute);
            CHECK(rendered == "class=\"big\"");
        }
    }

    TEST_CASE("Attribute can be created with const char") {
        const char value[] = "big";
        _class attribute{value};

        CHECK(std::string(attribute.name) == "class");
        CHECK(attribute.values[0].valueOwned == value);

        SUBCASE("Const char attribute can be rendered") {
            std::string rendered = render(attribute);
            CHECK(rendered == "class=\"big\"");
        }
    }

    TEST_CASE("Attribute can be created with std::string") {
        std::string value{"big"};
        _class attribute{value};

        CHECK(std::string(attribute.name) == "class");
        CHECK(attribute.values[0].valueOwned == value);

        SUBCASE("std::string attribute can be rendered") {
            std::string rendered = render(attribute);
            CHECK(rendered == "class=\"big\"");
        }
    }

    TEST_CASE("Attribute can be created with placeholder") {
        const std::string_view value{"replacable"};
        _class attribute{_{value}};

        CHECK(std::string(attribute.name) == "class");
        CHECK(attribute.values[0].valueOwned == value);

        SUBCASE("Placeholder attribute can be rendered") {
            std::string rendered = render(attribute);
            CHECK(rendered == "class=\"replacable\"");
        }
    }

    TEST_CASE("Attribute can be created with multiple values") {
        _class attribute{"big", "tall"};

        CHECK(std::string(attribute.name) == "class");
        CHECK(attribute.values[0].valueOwned == "big");
        CHECK(attribute.values[1].valueOwned == "tall");

        SUBCASE("Multiple values attribute can be rendered") {
            std::string rendered = render(attribute);
            CHECK(rendered == "class=\"big tall\"");
        }
    }

    TEST_CASE("Custom attribute can be defined") {
        _dataCustom customAttribute{"something"};

        CHECK(std::string(customAttribute.name) == "data-custom");
        CHECK(customAttribute.values[0].valueOwned == "something");

        SUBCASE("Custom attribute can be rendered") {
            std::string rendered = render(customAttribute);
            CHECK(rendered == "data-custom=\"something\"");
        }
    }
}

TEST_SUITE("Attributes") {
    using namespace Webxx;

    static constexpr char customDataAttr[] = "data-custom";
    using _dataCustom = attr<customDataAttr>;

    TEST_CASE("Attributes can be empty") {
        attrs attributes{};

        CHECK(attributes.size() == 0);

        SUBCASE("Empty attributes can be rendered") {
            std::string rendered = render(attributes);
            CHECK(rendered == "");
        }
    }

    TEST_CASE("Attributes can be populated via initializer") {
        attrs attributes{
            _class{"big small"},
            _id{"thing"},
            _disabled{},
            _dataCustom{"hello"}
        };

        CHECK(attributes.size() == 4);

        SUBCASE("Initializer populated attributes can be rendered") {
            std::string rendered = render(attributes);
            CHECK(rendered == " class=\"big small\" id=\"thing\" disabled data-custom=\"hello\"");
        }
    }

    TEST_CASE("Attributes can be populated via vector") {
        std::vector<internal::HtmlAttribute> attributesV{
            _class{"big small"},
            _id{"thing"},
            _disabled{},
            _dataCustom{"hello"},
        };

        attrs attributes{attributesV};

        CHECK(attributes.size() == 4);

        SUBCASE("Dynamically populated attributes can be rendered") {
            std::string rendered = render(attributes);
            CHECK(rendered == " class=\"big small\" id=\"thing\" disabled data-custom=\"hello\"");
        }
    }

    TEST_CASE("Attributes can be dynamically populated") {
        attrs attributes{};

        attributes.push_back(_class{"big small"});
        attributes.push_back(_id{"thing"});
        attributes.push_back(_disabled{});
        attributes.push_back(_dataCustom{"hello"});

        CHECK(attributes.size() == 4);

        SUBCASE("Dynamically populated attributes can be rendered") {
            std::string rendered = render(attributes);
            CHECK(rendered == " class=\"big small\" id=\"thing\" disabled data-custom=\"hello\"");
        }
    }
}

TEST_SUITE("Node") {
    using namespace Webxx;

    TEST_CASE("Node can be empty") {
        h1 node;

        CHECK(std::string(node.options.tagName) == "h1");

        SUBCASE("Empty node can be rendered") {
            CHECK(render(node) == "<h1></h1>");
        }
    }

    TEST_CASE("Node can have attributes") {
        h1 node{{_class{"title"}, _id{"theTitle"}}};

        SUBCASE("Node with attributes can be rendered") {
            CHECK(render(node) == "<h1 class=\"title\" id=\"theTitle\"></h1>");
        }
    }

    TEST_CASE("Node can have single text content") {
        h1 node{"Hello world"};

        SUBCASE("Node with single text content can be rendered") {
            CHECK(render(node) == "<h1>Hello world</h1>");
        }
    }

    TEST_CASE("Node can have multiple text content") {
        h1 node{"Hello", " world"};

        SUBCASE("Node with multiple text content can be rendered") {
            CHECK(render(node) == "<h1>Hello world</h1>");
        }
    }

    TEST_CASE("Node can have single child") {
        h1 node{a{"Hello world"}};

        SUBCASE("Node with single child can be rendered") {
            CHECK(render(node) == "<h1><a>Hello world</a></h1>");
        }
    }

    TEST_CASE("Node can have multiple children") {
        h1 node{a{"Hello"}, span{" world"}};

        SUBCASE("Node with multiple children can be rendered") {
            CHECK(render(node) == "<h1><a>Hello</a><span> world</span></h1>");
        }
    }

    TEST_CASE("Node can mix children and content") {
        h1 node{"Hello", a{" world"}, "!"};

        SUBCASE("Node with mixed children and content can be rendered") {
            CHECK(render(node) == "<h1>Hello<a> world</a>!</h1>");
        }
    }

    TEST_CASE("Node can have children from vector") {
        // @todo
    }

    TEST_CASE("Node can have attributes with children and content") {
        h1 node{{_class{"title"}, _id{"theTitle"}}, "Hello", a{" world"}, "!"};

        SUBCASE("Node with attributes and mixed children and content can be rendered") {
            CHECK(render(node) == "<h1 class=\"title\" id=\"theTitle\">Hello<a> world</a>!</h1>");
        }
    }

    TEST_CASE("Nodes can be arbitrarily nested") {
        title title{"Hey"};

        html html {
            head {
                title,
            },
            body {
                h1{{_class{"title"}},
                    "Hello!"
                },
                p{"Goodbye."},
            },
        };

        SUBCASE("Arbitrarily nested nodes can be rendered") {
            CHECK(render(html) == "<html><head><title>Hey</title></head><body><h1 class=\"title\">Hello!</h1><p>Goodbye.</p></body></html>");
        }
    }

    TEST_CASE("Node can be self-closing") {
        img node;

        CHECK(std::string(node.options.tagName) == "img");
        CHECK(node.options.selfClosing == true);

        SUBCASE("Self-closing node can be rendered") {
            CHECK(render(node) == "<img/>");
        }
    }

    TEST_CASE("Self-closing node can have attributes") {
        img node{{_class{"logo"}, _href{"/logo.gif"}}};

        SUBCASE("Self-closing node with attributes can be rendered") {
            CHECK(render(node) == "<img class=\"logo\" href=\"/logo.gif\"/>");
        }
    }

    TEST_CASE("Node can have prefix") {
        doc node;

        CHECK(std::string(node.options.tagName) == "");
        CHECK(std::string(node.options.prefix) == "<!doctype html>");

        SUBCASE("Prefixed node can be rendered") {
            CHECK(render(node) == "<!doctype html>");
        }
    }
}