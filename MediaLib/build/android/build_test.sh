#!/bin/bash -e
source ./.env.rc

build_test() {
    echo "Building test..."
    cd ${TEST_DIR}
    make
    make install
    cd ${THIS_PATH}
}

build_test

