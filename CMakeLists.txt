cmake_minimum_required(VERSION 3.13.0)

project(webxx VERSION 0.1.0)

set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MAIN_PROJECT ON)
endif()

if(NOT "${CMAKE_CXX_STANDARD}")
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED On)
endif()

option(WEBXX_BENCHMARK "Enable benchmarking targets (requires WEBXX_TEST=ON)" ${MAIN_PROJECT})
option(WEBXX_COVERAGE "Enable code coverage reporting (requires WEBXX_TEST=ON)" Off)
option(WEBXX_PEDANTIC "Enable extra checks" ${MAIN_PROJECT})
option(WEBXX_TEST "Enable test targets" ${MAIN_PROJECT})

set(WEBXX_NS ${PROJECT_NAME})
set(WEBXX_NAME ${PROJECT_NAME})
set(WEBXX_HEADERS include/${WEBXX_NAME}/${WEBXX_NAME}.h)

add_library(${WEBXX_NAME} INTERFACE)
add_library(${WEBXX_NS}::${WEBXX_NAME} ALIAS ${WEBXX_NAME})
target_include_directories(${WEBXX_NAME} INTERFACE ${PROJECT_SOURCE_DIR}/include)

set(PEDANTIC_FLAGS "")
if (WEBXX_PEDANTIC)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        set(PEDANTIC_FLAGS
            -pedantic-errors
            -Wall
            -Wextra
            -pedantic
            -Wold-style-cast
            -Wundef
            -Wredundant-decls
            -Wwrite-strings
            -Wpointer-arith
            -Wcast-qual
            -Wformat=2
            -Wmissing-include-dirs
            -Wcast-align
            -Wctor-dtor-privacy
            -Wdisabled-optimization
            -Winvalid-pch
            -Woverloaded-virtual
            -Wconversion
            -Wundef
            -Wno-ctor-dtor-privacy
            -Wno-format-nonliteral
            -Wno-dangling-else
            -Wno-unused-local-typedefs
            -Wdouble-promotion
            -Wtrampolines
            -Wzero-as-null-pointer-constant
            -Wuseless-cast
            -Wvector-operation-performance
            -Wsized-deallocation
            -Wshadow
            -Wshift-overflow=2
            -Wnull-dereference
            -Wduplicated-cond
            -Werror
        )
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(PEDANTIC_FLAGS
            -Wall
            -Wextra
            -pedantic
            -Wconversion
            -Wundef
            -Wdeprecated
            -Wweak-vtables
            -Wshadow
            -Wno-gnu-zero-variadic-macro-arguments
            -Wzero-as-null-pointer-constant
            -Werror
        )
    endif ()
endif()

if(WEBXX_TEST)
    include(CTest)
    add_subdirectory(test)
endif()
