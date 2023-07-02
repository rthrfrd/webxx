# Don't build and run benchmark's tests
set(BENCHMARK_ENABLE_GTEST_TESTS Off)
set(BENCHMARK_ENABLE_TESTING Off)
option(INJA_BUILD_TESTS Off)
option(BUILD_BENCHMARK Off)

# Fetch benchmark & inja
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG        v1.7.1
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(
    inja
    GIT_REPOSITORY https://github.com/pantor/inja.git
    GIT_TAG        v3.3.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(benchmark inja)

# Build an executable against benchmark:
set(BENCHMARK_MAIN ${WEBXX_NS}_benchmark)
add_executable(${BENCHMARK_MAIN} benchmark.cpp)
target_compile_features(${BENCHMARK_MAIN} PRIVATE cxx_std_17)
target_include_directories(
    ${WEBXX_NAME} INTERFACE
    ${PROJECT_SOURCE_DIR}/include
    ${inja_SOURCE_DIR}/include
)
target_link_libraries(${BENCHMARK_MAIN} PRIVATE benchmark::benchmark inja webxx)

# Add it to our test runner:
add_test(NAME Benchmark COMMAND ${BENCHMARK_MAIN} --benchmark_format=json)
