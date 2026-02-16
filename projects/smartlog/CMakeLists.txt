cmake_minimum_required(VERSION 3.16)
project(smartlog VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(smartlog
    src/smartlog_core.c
    src/utils.c
)

target_include_directories(smartlog
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(smartlog
    PRIVATE
        -Wall
        -Wextra
        -Wpedantic
)

add_executable(mini_log src/mini_log.c)
target_link_libraries(mini_log PRIVATE smartlog)

include(CTest)
if(BUILD_TESTING)
    add_executable(smartlog_tests
        test/test_smartlog.c
        src/smartlog_core.c
        src/utils.c
    )
    target_include_directories(smartlog_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_compile_options(smartlog_tests
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
    target_compile_definitions(smartlog_tests PRIVATE SMARTLOG_TEST_FAULTS=1)
    add_test(NAME smartlog_tests COMMAND smartlog_tests)
endif()

install(TARGETS smartlog mini_log)
install(DIRECTORY include/ DESTINATION include)
