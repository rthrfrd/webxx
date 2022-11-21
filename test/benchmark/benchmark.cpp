#include "benchmark/benchmark.h"
#include "inja/inja.hpp"
#include "webxx.h"

using namespace Webxx;

typedef const char* Input;
constexpr static char helloWorld[]{"Hello world."};
constexpr static char something[]{"something"};
constexpr static char somethingElse[]{"Something else."};


////|                  |////
////|  Single element  |////
////|                  |////


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

std::string renderSingleElementSprintf (Input input) {
    char buffer[64];
    sprintf(buffer, "<h1>%s</h1>", input);
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


std::string renderMultiElementWebxx (Input a, Input b, Input c) {
    return render(dv{{_class{b}},
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

std::string renderMultiElementSprintf (Input a, Input b, Input c) {
    char buffer[128];
    sprintf(buffer, "<div class=\"%s\"><h1>%s</h1><p>%s</p></div>", b, a, c);
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



BENCHMARK_MAIN();
