include(FetchContent)

# Fetch dependencies:
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.9
    GIT_SHALLOW    TRUE
)
FetchContent_Declare(
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG        9.1.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(doctest fmt)

# Use xcode toolchain on mac where available:
find_program(MAYBE_XCRUN xcrun)
if(MAYBE_XCRUN MATCHES "NOTFOUND")
    set(MAYBE_XCRUN "")
endif()

# Generate test coverage configuration:
set(TEST_COVERAGE ${WEBXX_NS}_test_coverage)
add_library(${TEST_COVERAGE} INTERFACE)
if(WEBXX_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${TEST_COVERAGE} INTERFACE -O0 -g --coverage)
        target_link_options(${TEST_COVERAGE} INTERFACE --coverage)
    endif()
endif()
target_link_libraries(${WEBXX_NAME} INTERFACE ${TEST_COVERAGE})

# Generate test entrypoint:
set(TEST_MAIN ${WEBXX_NS}_test_main)
set(TEST_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)

add_library(${TEST_MAIN} main.cpp)
target_link_libraries(${TEST_MAIN} PUBLIC doctest fmt ${WEBXX_NAME})
target_include_directories(${TEST_MAIN} PUBLIC ${TEST_INCLUDE_DIRS})
target_compile_options(${TEST_MAIN} PUBLIC ${PEDANTIC_FLAGS})

# Use all test types:
set(TEST_TYPES unit; integration; example)
foreach(TEST_TYPE ${TEST_TYPES})
    # Find tests for type:
    file(GLOB TEST_FILES ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_TYPE}/*.cpp)

    foreach(TEST_FILE ${TEST_FILES})
        get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)

        # Generate executable for each test:
        add_executable(${TEST_NAME} ${TEST_FILE})
        target_link_libraries(${TEST_NAME} PRIVATE ${TEST_MAIN})

        # Add test:
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
        set_tests_properties(${TEST_NAME} PROPERTIES LABELS "all")

        if(WEBXX_COVERAGE)
            string(REGEX MATCH ^[0-9]+ COMPILER_MAJOR_VERSION ${CMAKE_CXX_COMPILER_VERSION})
            if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                # This relies on a CMake implementation detail:
                set(TEST_OBJECT_DIR ${CMAKE_BINARY_DIR}/test/CMakeFiles/${TEST_NAME}.dir/${TEST_TYPE})
                # Transform LLVM output to gcov coverage format:
                add_test(NAME ${TEST_NAME}LlvmCov
                    COMMAND ${MAYBE_XCRUN} llvm-cov-${COMPILER_MAJOR_VERSION} gcov ./${TEST_NAME}.cpp.gcno
                    WORKING_DIRECTORY ${TEST_OBJECT_DIR}
                )
                set_tests_properties(${TEST_NAME}LlvmCov PROPERTIES DEPENDS ${TEST_NAME})
            endif()
        endif()
    endforeach()
endforeach()

# Generate benchmarking tests:
if(WEBXX_BENCHMARK)
    add_subdirectory(benchmark)
endif()
