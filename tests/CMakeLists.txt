file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp")

set(TEST_BIN_DIR "${CMAKE_BINARY_DIR}/tests/bin")

foreach (TEST_SRC IN LISTS TEST_SOURCES)
    file(RELATIVE_PATH REL "${CMAKE_CURRENT_SOURCE_DIR}" "${TEST_SRC}")
    get_filename_component(BASENAME "${REL}" NAME_WE)
    get_filename_component(DIRNAME "${REL}" DIRECTORY)

    string(REPLACE "/" "_" TARGET_NAME "t_${DIRNAME}_${BASENAME}")
    string(REPLACE "\\" "_" TARGET_NAME "${TARGET_NAME}")

    add_executable(${TARGET_NAME} "${TEST_SRC}")
    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_20)
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra -Wpedantic)

    target_include_directories(${TARGET_NAME} PRIVATE
            "${CMAKE_SOURCE_DIR}/include"
            "${CMAKE_SOURCE_DIR}/external/compiler_ir/include"
    )
    target_link_libraries(${TARGET_NAME} PRIVATE frontend_utils compiler_ir)
    if (magic_enum_FOUND)
        target_link_libraries(${TARGET_NAME} PRIVATE magic_enum::magic_enum)
    endif ()


    set_target_properties(${TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${TEST_BIN_DIR}"
            FOLDER "tests"
    )

    add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})
    set_tests_properties(${TARGET_NAME} PROPERTIES LABELS "unit;test")
endforeach ()

# Integration: compile sample to IR and execute via lli to verify runtime result.
set(TEST_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
add_test(
        NAME integration_lli_arith
        COMMAND /bin/sh -c
        "$<TARGET_FILE:cmm> -o ${TEST_BIN_DIR}/lli_arith.ll ${TEST_DATA_DIR}/lli_arith.sy && lli ${TEST_BIN_DIR}/lli_arith.ll; rc=\$?; test \$rc -eq 7"
)
set_tests_properties(integration_lli_arith PROPERTIES LABELS "integration;ir")

# Compare move traces against provided lab4 references (ignoring reductions/special symbols).
set(LAB4_TEST_DIR "${CMAKE_SOURCE_DIR}/tests/lab4")
foreach (CASE IN ITEMS test1 test2 test3)
    add_test(
            NAME lab4_trace_${CASE}
            COMMAND /bin/sh -c
             "cat ${LAB4_TEST_DIR}/${CASE}.sy | $<TARGET_FILE:cmm> --gtrace-only - | ${CMAKE_SOURCE_DIR}/tests/compare_trace.py ${LAB4_TEST_DIR}/${CASE}.ref"
    )
    set_tests_properties(lab4_trace_${CASE} PROPERTIES LABELS "integration;trace")
endforeach ()

# Lab2 token-level regression (lexer only)
set(LAB2_TEST_DIR "${CMAKE_SOURCE_DIR}/tests/lab2")
foreach (CASE IN ITEMS test1 test2 test3)
    add_test(
            NAME lab2_lex_${CASE}
            COMMAND /bin/sh -c
            "cat ${LAB2_TEST_DIR}/${CASE}.sy | $<TARGET_FILE:cmm> --lex-only - | ${CMAKE_SOURCE_DIR}/tests/compare_tokens.py ${LAB2_TEST_DIR}/${CASE}.ref"
    )
    set_tests_properties(lab2_lex_${CASE} PROPERTIES LABELS "integration;lexer")
endforeach ()
