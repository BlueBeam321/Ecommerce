#!/bin/bash -e

source ./.env.rc

build_access() {
    echo "Building access..."
    cd ${MODULE_DIR}/access
    make
    make install
    cd ${THIS_PATH}
}

build_packetizer() {
    echo "Building packetizer..."
    cd ${MODULE_DIR}/packetizer
    make
    make install
    cd ${THIS_PATH}
}

build_codec() {
    echo "Building codec..."
    cd ${MODULE_DIR}/codec
    make
    make install
    cd ${THIS_PATH}
}

build_mux() {
    echo "Building mux..."
    cd ${MODULE_DIR}/mux
    make
    make install
    cd ${THIS_PATH}
}

build_demux() {
    echo "Building demux..."
    cd ${MODULE_DIR}/demux
    make
    make install
    cd ${THIS_PATH}
}

build_metaengine() {
    echo "Building metaengine..."
    cd ${MODULE_DIR}/meta_engine
    make
    make install
    cd ${THIS_PATH}
}

build_audiofilter() {
    echo "Building audiofilter..."
    cd ${MODULE_DIR}/audio_filter
    make
    make install
    cd ${THIS_PATH}
}

build_audiooutput() {
    echo "Building audiooutput..."
    cd ${MODULE_DIR}/audio_output
    make
    make install
    cd ${THIS_PATH}
}

build_videochroma() {
    echo "Building videochroma..."
    cd ${MODULE_DIR}/video_chroma
    make
    make install
    cd ${THIS_PATH}
}

build_videofilter() {
    echo "Building videofilter..."
    cd ${MODULE_DIR}/video_filter
    make
    make install
    cd ${THIS_PATH}
}

build_videooutput() {
    echo "Building videooutput..."
    cd ${MODULE_DIR}/video_output
    make
    make install
    cd ${THIS_PATH}
}

build_textrenderer() {
    echo "Building textrenderer..."
    cd ${MODULE_DIR}/text_renderer
    make
    make install
    cd ${THIS_PATH}
}

build_streamoutput() {
    echo "Building streamoutput..."
    cd ${MODULE_DIR}/stream_output
    make
    make install
    cd ${THIS_PATH}
}

build_crypt() {
    echo "Building crypt..."
    cd ${MODULE_DIR}/crypt
    make
    make install
    cd ${THIS_PATH}
}

build_access
build_packetizer
build_codec
build_mux
build_demux
build_metaengine
build_audiofilter
build_audiooutput
build_videochroma
build_videofilter
build_videooutput
build_textrenderer
build_streamoutput
build_crypt

