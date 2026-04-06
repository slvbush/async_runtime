cmake_minimum_required(VERSION 3.24...4.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ConfigureCompiler)
include(CompilerWarnings)

project(async-runtime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

ct_configure_compiler()
include(Dependencies)

# Setup a 'lib' target
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS lib/*.cpp)
list(LENGTH LIB_SOURCES LIB_SOURCES_LENGTH)
if(LIB_SOURCES_LENGTH EQUAL 0)
    add_library(lib INTERFACE)
    target_include_directories(lib INTERFACE lib)
else()
    add_library(lib ${LIB_SOURCES})
    target_include_directories(lib PUBLIC lib)
    set_target_properties(lib PROPERTIES LINKER_LANGUAGE CXX)
    ct_set_compiler_warnings(lib)
endif()

# Setup a 'tests' target
file(GLOB_RECURSE TESTS_SOURCES CONFIGURE_DEPENDS test/*.cpp)
add_executable(tests ${TESTS_SOURCES})
target_include_directories(tests PRIVATE test)
ct_set_compiler_warnings(tests)

# Link tests with lib
target_link_libraries(tests PRIVATE lib)

# Link tests with dependencies
find_package(GTest CONFIG REQUIRED)
target_link_libraries(tests PRIVATE GTest::gtest GTest::gtest_main)

