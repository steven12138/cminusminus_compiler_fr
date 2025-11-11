cmake_minimum_required(VERSION 3.20)

project(c_minus_frontend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

# for vscode extensions
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

add_executable(compiler_frontend ${FRONTEND_SOURCES})

target_include_directories(
        compiler_frontend
        PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/compiler_ir/include")

target_link_libraries(compiler_frontend PRIVATE compiler_ir)

target_compile_options(compiler_frontend PRIVATE -Wall -Wextra -Wpedantic)

option(BUILD_TESTS "Build test executables under tests/*/*.cpp" ON)
if (BUILD_TESTS)
    add_subdirectory(tests)
endif ()