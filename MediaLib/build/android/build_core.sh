#!/bin/bash -e

source ./.env.rc

build_compat() {
    echo "Building compat..."
    cd ${CORE_DIR}/compat
    make
    make install
    cd ${THIS_PATH}
}

build_misc() {
    echo "Building misc..."
    
    cd ${CORE_DIR}/misc
    make
    make install
    cd ${THIS_PATH}
}

build_videooutput() {
    echo "Building videooutput..."
    
    cd ${CORE_DIR}/video_output
    make
    make install
    cd ${THIS_PATH}
}

build_text() {
    echo "Building text..."
    cd ${CORE_DIR}/text
    make
    make install
    cd ${THIS_PATH}
}

build_input() {
    echo "Building input..."
    cd ${CORE_DIR}/input
    make
    make install
    cd ${THIS_PATH}
}

build_modules() {
    echo "Building modules..."
    cd ${CORE_DIR}/modules
    make
    make install
    cd ${THIS_PATH}
}

build_config() {
    echo "Building config..."
    cd ${CORE_DIR}/config
    make
    make install
    cd ${THIS_PATH}
}

build_network() {
    echo "Building network..."
    cd ${CORE_DIR}/network
    make
    make install
    cd ${THIS_PATH}
}

build_playlist() {
    echo "Building playlist..."
    cd ${CORE_DIR}/playlist
    make
    make install
    cd ${THIS_PATH}
}

build_audiooutput() {
    echo "Building audiooutput..."
    cd ${CORE_DIR}/audio_output
    make
    make install
    cd ${THIS_PATH}
}

build_streamoutput() {
    echo "Building streamoutput..."
    cd ${CORE_DIR}/stream_output
    make
    make install
    cd ${THIS_PATH}
}

build_compat
build_misc
build_videooutput
build_text
build_input
build_modules
build_config
build_network
build_playlist
build_audiooutput
build_streamoutput

