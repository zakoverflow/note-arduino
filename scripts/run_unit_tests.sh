#!/bin/bash

COVERAGE=0
MEM_CHECK=0
VERBOSE=0

# Parse command line arguments.
while [[ "$#" -gt 0 ]]; do
    case $1 in
        --coverage) COVERAGE=1 ;;
        --mem-check) MEM_CHECK=1 ;;
        --verbose) VERBOSE=1 ;;
        *) echo "Unknown parameter: $1"; exit 1 ;;
    esac
    shift
done

# Determine the directory of the script.
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
ROOT_SRC_DIR="$SCRIPT_DIR/.."
TEST_SRC_DIR="$ROOT_SRC_DIR/test"

# Check for a CMakeLists.txt file, to ensure CMake operations
# are executed in the correct directory.
if [[ ! -f "$TEST_SRC_DIR/CMakeLists.txt" ]]; then
    echo "ERROR: Unable to locate ${TEST_SRC_DIR}/CMakeLists.txt. Expecting to use '../test' to access the test directory. Did the location of run_unit_tests.sh change?"
    exit 1
fi

# Move to the root source directory.
pushd $ROOT_SRC_DIR > /dev/null

CMAKE_OPTIONS=""
BUILD_OPTIONS=""
CTEST_OPTIONS=""
if [[ $COVERAGE -eq 1 ]]; then
    CMAKE_OPTIONS="${CMAKE_OPTIONS} -DNOTE_ARDUINO_COVERAGE:BOOL=ON"
    BUILD_OPTIONS="${BUILD_OPTIONS} coverage"
fi
if [[ $MEM_CHECK -eq 1 ]]; then
    CMAKE_OPTIONS="${CMAKE_OPTIONS} -DNOTE_ARDUINO_MEM_CHECK:BOOL=ON"
    CTEST_OPTIONS="${CTEST_OPTIONS} -T memcheck"

    # This fixes a problem when running valgrind in a Docker container when the
    # host machine is running Fedora. See https://stackoverflow.com/a/75293014.
    ulimit -n 1024
fi
if [[ $VERBOSE -eq 1 ]]; then
    CMAKE_OPTIONS="${CMAKE_OPTIONS} -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON --log-level=VERBOSE"
fi

# Configure the build.
cmake -B build/ -S test/ $CMAKE_OPTIONS
if [[ $? -ne 0 ]]; then
    echo "ERROR: CMake failed to configure build."
    popd > /dev/null
    exit 1
fi

# Build the unit-tests (and, with --coverage, run them and write
# coverage/lcov.info). Set JOBS to limit build parallelism (default: -j).
cmake --build build/ -- $BUILD_OPTIONS -j${JOBS}
if [[ $? -ne 0 ]]; then
    echo "ERROR: CMake failed to build the unit-tests."
    popd > /dev/null
    exit 1
fi

# Run the unit tests.
ctest --test-dir build/ --output-on-failure ${CTEST_OPTIONS}
if [[ $? -ne 0 ]]; then
    echo "ERROR: Unit-tests did not pass."
    popd > /dev/null
    exit 1
fi

echo "SUCCESS: Tests passed."
popd > /dev/null
