#!/bin/bash -e
source ./.env.rc

build_libvlc() {
    echo "Building libvlc..."
    cd ${LIBVLC_DIR}
    make
    make install
    cd ${THIS_PATH}
}

build_libvlc

