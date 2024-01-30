#include "benchmark/benchmark.h"
#include "inja/inja.hpp"
#include "webxx.h"

#include <array>

using namespace Webxx;


typedef const char* Input;
constexpr static const char* helloWorld{"Hello world."};
constexpr static const char* something{"something"};
constexpr static const char* somethingElse{"Something else."};
constexpr size_t nMany{1000};
constexpr std::array<size_t,nMany> nItems{};

////|                  |////
////|  Single element  |////
////|                  |////


std::string renderSingleElementInja (Input input) {
    inja::json data;
    data["input"] = input;
    return inja::render("<h1>{{ input }}</h1>", data);
}

static void singleElementInja (benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderSingleElementInja(helloWorld));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(singleElementInja);

std::string renderSingleElementWebxx (Input input) {
    return render(h1{input});
}

static void singleElementWebxx (benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderSingleElementWebxx(helloWorld));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(singleElementWebxx);

std::string renderSingleElementSprintf (Input input) {
    char buffer[64];
    snprintf(buffer, 64, "<h1>%s</h1>", input);
    return buffer;
}

static void singleElementSprintf (benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderSingleElementSprintf(helloWorld));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(singleElementSprintf);

std::string renderSingleElementStringAppend (Input input) {
    std::string html;
    html.append("<h1>");
    html.append(input);
    html.append("</h1>");
    return html;
}

static void singleElementStringAppend (benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderSingleElementStringAppend(helloWorld));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(singleElementStringAppend);


////|                 |////
////|  Multi-element  |////
////|                 |////


std::string renderMultiElementInja (Input a, Input b, Input c) {
    inja::json data;
    data["a"] = a;
    data["b"] = b;
    data["c"] = c;
    return inja::render("<div class=\"{{ b }}\"><h1>{{ a }}</h1><p>{{ c }}</p></div>", data);
}

static void multiElementInja (benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderMultiElementInja(helloWorld, something, somethingElse));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(multiElementInja);

std::string renderMultiElementWebxx (Input a, Input b, Input c) {
    return render(dv{_class{b},
        h1{a},
        p{c},
    });
}

static void multiElementWebxx (benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderMultiElementWebxx(helloWorld, something, somethingElse));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(multiElementWebxx);

std::string renderMultiElementSprintf (Input a, Input b, Input c) {
    char buffer[128];
    snprintf(buffer, 128, "<div class=\"%s\"><h1>%s</h1><p>%s</p></div>", b, a, c);
    return buffer;
}

static void multiElementSprintf (benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderMultiElementSprintf(helloWorld, something, somethingElse));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(multiElementSprintf);

std::string renderMultiStringAppend (Input a, Input b, Input c) {
    std::string html;
    html.append("<div class=\"");
    html.append(b);
    html.append("\">");
    html.append("<h1>");
    html.append(a);
    html.append("</h1>");
    html.append("<p>");
    html.append(c);
    html.append("</p>");
    html.append("</div>");
    return html;
}

static void multiElementStringAppend (benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(renderMultiStringAppend(helloWorld, something, somethingElse));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(multiElementStringAppend);


////|                |////
////|  Many-element  |////
////|                |////


std::string render1kElementInja (Input a, Input b, Input c) {
    inja::json data;

    data["n"] = nItems;
    data["a"] = a;
    data["b"] = b;
    data["c"] = c;
    return inja::render(
        "<ol>"
        "{% for i in n %}"
        "<li class=\"{{ b }}\">{{ loop.index }}<h1>{{ a }}</h1><p>{{ c }}</p></li>"
        "{% endfor %}"
        "</ol>"
    , data);
}

static void loop1kInja (benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(render1kElementInja(helloWorld, something, somethingElse));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(loop1kInja);

std::string render1kElementWebxx (Input a, Input b, Input c) {
    return render(ol{
        loop(nItems, [a,b,c] (const auto&, const Loop& loop) -> auto {
            return li{_class{b},
                std::to_string(loop.index),
                h1{a},
                p{c},
            };
        }),
    });
}

static void loop1kWebxx (benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(render1kElementWebxx(helloWorld, something, somethingElse));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(loop1kWebxx);

std::string render1kStringAppend (Input a, Input b, Input c) {
    std::string html;

    html.append("<ol>");
    int i{0};
    for (auto& _ : nItems) {
        html.append("<li class=\"");
        html.append(b);
        html.append("\">");
        html.append(std::to_string(i)),
        html.append("<h1>");
        html.append(a);
        html.append("</h1>");
        html.append("<p>");
        html.append(c);
        html.append("</p>");
        html.append("</li>");
        ++i;
    }
    html.append("</ol>");

    return html;
}

static void loop1kStringAppend (benchmark::State& state) {
    for (auto _ : state) {
        benchmark::DoNotOptimize(render1kStringAppend(helloWorld, something, somethingElse));
        benchmark::ClobberMemory();
    }
}
BENCHMARK(loop1kStringAppend);


BENCHMARK_MAIN();
