cmake_minimum_required(VERSION 3.20)

project(ipmi-fan-control VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(IPMI_FAN_CONTROL_BUILD_TESTS "构建测试 / Build tests" ON)

# Derive short git SHA for version suffix; fall back safely when .git is absent.
find_package(Git QUIET)
set(_GIT_SHA "unknown")
if(Git_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE _GIT_SHA
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _GIT_RESULT
    )
    if(NOT _GIT_RESULT EQUAL 0)
        set(_GIT_SHA "unknown")
    endif()
endif()
set(IPMI_FAN_CONTROL_VERSION "v${PROJECT_VERSION}-git-${_GIT_SHA}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ipmi_fan_control/version.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/ipmi_fan_control/version.hpp
    @ONLY
)

add_library(ipmi_fan_control_lib
    src/cli.cpp
    src/config.cpp
    src/control.cpp
    src/ipmi.cpp
    src/logger.cpp
    src/process.cpp
    src/service.cpp
)

target_include_directories(ipmi_fan_control_lib
    PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(ipmi_fan_control_lib PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
elseif(MSVC)
    target_compile_options(ipmi_fan_control_lib PRIVATE /W4 /permissive-)
endif()

add_executable(ipmi-fan-control src/main.cpp)
target_link_libraries(ipmi-fan-control PRIVATE ipmi_fan_control_lib)

include(GNUInstallDirs)
install(TARGETS ipmi-fan-control RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES examples/config.example.ini DESTINATION ${CMAKE_INSTALL_DATADIR}/ipmi-fan-control)
install(FILES etc/systemd/system/ipmi-fan-control.service.example DESTINATION ${CMAKE_INSTALL_DATADIR}/ipmi-fan-control)

if(IPMI_FAN_CONTROL_BUILD_TESTS)
    enable_testing()

    add_executable(ipmi_fan_control_tests tests/test_main.cpp)
    target_link_libraries(ipmi_fan_control_tests PRIVATE ipmi_fan_control_lib)
    add_test(NAME ipmi_fan_control_tests COMMAND ipmi_fan_control_tests)
endif()
