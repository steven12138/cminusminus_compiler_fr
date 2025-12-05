cmake_minimum_required(VERSION 3.20)

project(c_minus_frontend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

set(TARGET_NAME "cmm")

# Enable CTest at the top level so ctest can discover subdirectory tests.
include(CTest)
enable_testing()

# for vscode extensions
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(magic_enum CONFIG QUIET)
if (magic_enum_FOUND)
    message(STATUS "magic_enum found: enabling pretty enums name")
    add_compile_definitions(USE_MAGIC_ENUM)
else ()
    message(STATUS "magic_enum not found, use integer values for enums")
endif ()

option(ENABLE_TIMING "Enable timing of component initialization" OFF)
if (ENABLE_TIMING)
    add_compile_definitions(TIMING)
    message(STATUS "Timing enabled")
else ()
    message(STATUS "Timing disabled")
endif ()

file(GLOB IR_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/external/compiler_ir/src/*.cpp")

add_library(compiler_ir STATIC ${IR_SOURCES})
target_include_directories(compiler_ir PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/external/compiler_ir/include")

file(GLOB_RECURSE FRONTEND_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(frontend_utils STATIC ${FRONTEND_SOURCES})
target_include_directories(frontend_utils PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/compiler_ir/include"
)

add_executable(${TARGET_NAME} ${FRONTEND_SOURCES})

target_include_directories(
        ${TARGET_NAME}
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/compiler_ir/include")

target_link_libraries(${TARGET_NAME} PRIVATE compiler_ir)

if (MSVC)
    target_compile_options(${TARGET_NAME} PRIVATE /W4 /permissive-)
else ()
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif ()

option(BUILD_TESTS "Build test executables under tests/*/*.cpp" ON)
if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()
