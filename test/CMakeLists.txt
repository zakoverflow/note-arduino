cmake_minimum_required(VERSION 3.20)

project(NoteArduinoUnitTests LANGUAGES C CXX)

option(NOTE_ARDUINO_COVERAGE "Compile for test coverage reporting." OFF)
option(NOTE_ARDUINO_MEM_CHECK "Run tests with Valgrind." OFF)

include(FetchContent)

# Catch2 v3 requires C++14.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message(STATUS "Downloading Catch2...")
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.3.0
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

# MEMORYCHECK_COMMAND_OPTIONS must be set before including CTest.
# See: https://stackoverflow.com/a/60741757
if(NOTE_ARDUINO_MEM_CHECK)
    find_program(VALGRIND valgrind REQUIRED)
    message(STATUS "Found valgrind: ${VALGRIND}")
    set(MEMORYCHECK_COMMAND_OPTIONS "--leak-check=full --error-exitcode=66")
endif(NOTE_ARDUINO_MEM_CHECK)
include(CTest)
include(Catch)

set(NOTE_ARDUINO_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(NOTE_ARDUINO_SRC_DIR ${NOTE_ARDUINO_ROOT_DIR}/src)
set(FAKES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/fakes)
set(ARDUINO_FAKES ${FAKES_DIR}/arduino_fakes.cpp)
set(NOTE_C_FAKES ${FAKES_DIR}/note_c_fakes.cpp)
set(NOTE_ARDUINO_FAKES ${FAKES_DIR}/note_arduino_fakes.cpp)

set(TEST_TARGETS "")

# add_test(<name> SOURCES <files under test and fakes> [DEFINITIONS <defs>]
#          [TEST_SOURCE <basename in test/src>] [TEST_SUFFIX <ctest suffix>])
#
# Builds one Catch2 executable per unit, like note-c. TEST_SOURCE defaults to
# <name>; it lets one test file be built more than once with different flags.
macro(add_test TEST_NAME)
    cmake_parse_arguments(ARG "" "TEST_SOURCE;TEST_SUFFIX" "SOURCES;DEFINITIONS" ${ARGN})
    if(NOT ARG_TEST_SOURCE)
        set(ARG_TEST_SOURCE ${TEST_NAME})
    endif()
    add_executable(
        ${TEST_NAME}
        ${CMAKE_CURRENT_SOURCE_DIR}/src/${ARG_TEST_SOURCE}.cpp
        ${ARG_SOURCES}
    )
    # test/include comes first so its Arduino.h, Wire.h and note-c/note.h
    # stand-ins are found before anything in src/.
    target_include_directories(${TEST_NAME}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE ${NOTE_ARDUINO_SRC_DIR}
    )
    target_compile_definitions(${TEST_NAME}
        PRIVATE FFF_ARG_HISTORY_LEN=256u
        PRIVATE FFF_CALL_HISTORY_LEN=256u
        PRIVATE ${ARG_DEFINITIONS}
    )
    target_link_libraries(${TEST_NAME}
        PRIVATE Catch2::Catch2WithMain
    )
    # -Wno-deprecated-declarations: some note-arduino APIs (e.g. logDebug)
    # are deprecated, but they still have unit tests.
    target_compile_options(${TEST_NAME}
        PRIVATE -Wall
        PRIVATE -Wextra
        PRIVATE -Werror
        PRIVATE -Wpedantic
        PRIVATE -Wno-deprecated-declarations
        PRIVATE -O0
        PRIVATE -ggdb
    )
    if(NOTE_ARDUINO_COVERAGE)
        target_compile_options(${TEST_NAME} PRIVATE --coverage)
        target_link_options(${TEST_NAME} PRIVATE --coverage)
    endif()

    list(APPEND TEST_TARGETS ${TEST_NAME})

    if(ARG_TEST_SUFFIX)
        catch_discover_tests(${TEST_NAME} TEST_SUFFIX ${ARG_TEST_SUFFIX})
    else()
        catch_discover_tests(${TEST_NAME})
    endif()
endmacro(add_test)

add_test(Notecard_test
    SOURCES
        ${NOTE_ARDUINO_SRC_DIR}/Notecard.cpp
        ${ARDUINO_FAKES}
        ${NOTE_C_FAKES}
        ${NOTE_ARDUINO_FAKES}
)
add_test(NoteI2c_Arduino_test
    SOURCES
        ${NOTE_ARDUINO_SRC_DIR}/NoteI2c_Arduino.cpp
        ${ARDUINO_FAKES}
        ${NOTE_C_FAKES}
)
add_test(NoteI2c_Arduino_WIRE_HAS_END_test
    TEST_SOURCE NoteI2c_Arduino_test
    TEST_SUFFIX " [WIRE_HAS_END]"
    SOURCES
        ${NOTE_ARDUINO_SRC_DIR}/NoteI2c_Arduino.cpp
        ${ARDUINO_FAKES}
        ${NOTE_C_FAKES}
    DEFINITIONS
        WIRE_HAS_END
)
add_test(NoteLog_Arduino_test
    SOURCES
        ${NOTE_ARDUINO_SRC_DIR}/NoteLog_Arduino.cpp
        ${ARDUINO_FAKES}
)
add_test(NoteSerial_Arduino_test
    SOURCES
        ${NOTE_ARDUINO_SRC_DIR}/NoteSerial_Arduino.cpp
        ${ARDUINO_FAKES}
        ${NOTE_C_FAKES}
)
add_test(NoteTxn_Arduino_test
    SOURCES
        ${NOTE_ARDUINO_SRC_DIR}/NoteTxn_Arduino.cpp
        ${ARDUINO_FAKES}
        ${NOTE_C_FAKES}
)

if(NOTE_ARDUINO_COVERAGE)
    find_program(LCOV lcov REQUIRED)
    message(STATUS "Found lcov: ${LCOV}")

    # Run the tests, then write <repo>/coverage/lcov.info for Coveralls.
    add_custom_target(
        coverage
        COMMAND lcov --zerocounters --directory ${CMAKE_BINARY_DIR}
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    # We don't care about coverage for the tests, the fakes or Catch2.
    set(LCOV_EXCLUDE_LIST --exclude "*/test/*" --exclude "*/build/_deps/*")
    add_custom_command(
        TARGET coverage POST_BUILD
        COMMAND mkdir -p ${NOTE_ARDUINO_ROOT_DIR}/coverage
        COMMAND lcov --capture --no-external
                --base-directory ${NOTE_ARDUINO_ROOT_DIR}
                --directory ${CMAKE_BINARY_DIR}
                --rc branch_coverage=1 ${LCOV_EXCLUDE_LIST}
                --output-file ${NOTE_ARDUINO_ROOT_DIR}/coverage/lcov.info
        COMMAND lcov --summary --rc branch_coverage=1
                ${NOTE_ARDUINO_ROOT_DIR}/coverage/lcov.info
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    # The tests have to be built before we can generate the coverage report.
    add_dependencies(coverage ${TEST_TARGETS})
endif(NOTE_ARDUINO_COVERAGE)
