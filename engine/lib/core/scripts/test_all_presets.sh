#!/bin/bash

clean_build_dir() {
    mkdir -p build
    rm -rf build/*
}

check_exit_code() {
    if [ $? -ne 0 ]; then
        echo "Build failed"
        exit 1
    fi
}

build_preset() {
    echo "BUILD WITH PRESET = $1"
    cmake .. --preset "$1"
    check_exit_code
    make -j
    check_exit_code
}

test_preset() {
    clean_build_dir

    pushd build

    build_preset "$1"

    echo "RUN TESTS IN PRESET = $1"
    ctest --output-on-failure
    check_exit_code

    popd # go back to root
}

main() {
    test_preset "default"
    test_preset "default release"
    test_preset "debug test static"
    test_preset "debug test shared"
    test_preset "release test static"
    test_preset "release test shared"
}

main
