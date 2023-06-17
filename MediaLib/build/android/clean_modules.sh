
source ./.env.rc

clean_access() {
	echo "Cleaning access..."
    cd ${MODULE_DIR}/access
    make clean
    cd ${THIS_PATH}
}

clean_packetizer() {
	echo "Cleaning packetizer..."
    cd ${MODULE_DIR}/packetizer
    make clean
    cd ${THIS_PATH}
}

clean_codec() {
	echo "Cleaning codec..."
    cd ${MODULE_DIR}/codec
    make clean
    cd ${THIS_PATH}
}

clean_mux() {
	echo "Cleaning mux..."
    cd ${MODULE_DIR}/mux
    make clean
    cd ${THIS_PATH}
}

clean_demux() {
	echo "Cleaning demux..."
    cd ${MODULE_DIR}/demux
    make clean
    cd ${THIS_PATH}
}

clean_metaengine() {
	echo "Cleaning metaengine..."
    cd ${MODULE_DIR}/meta_engine
    make clean
    cd ${THIS_PATH}
}

clean_audiofilter() {
	echo "Cleaning audiofilter..."
    cd ${MODULE_DIR}/audio_filter
    make clean
    cd ${THIS_PATH}
}

clean_audiooutput() {
	echo "Cleaning audiooutput..."
    cd ${MODULE_DIR}/audio_output
    make clean
    cd ${THIS_PATH}
}

clean_videochroma() {
	echo "Cleaning videochroma..."
    cd ${MODULE_DIR}/video_chroma
    make clean
    cd ${THIS_PATH}
}

clean_videofilter() {
	echo "Cleaning videofilter..."
    cd ${MODULE_DIR}/video_filter
    make clean
    cd ${THIS_PATH}
}

clean_videooutput() {
	echo "Cleaning videooutput..."
    cd ${MODULE_DIR}/video_output
    make clean
    cd ${THIS_PATH}
}

clean_textrenderer() {
	echo "Cleaning textrenderer..."
    cd ${MODULE_DIR}/text_renderer
    make clean
    cd ${THIS_PATH}
}

clean_streamoutput() {
	echo "Cleaning streamoutput..."
    cd ${MODULE_DIR}/stream_output
    make clean
    cd ${THIS_PATH}
}

clean_crypt() {
	echo "Cleaning crypt..."
    cd ${MODULE_DIR}/crypt
    make clean
    cd ${THIS_PATH}
}

clean_access
clean_packetizer
clean_codec
clean_mux
clean_demux
clean_metaengine
clean_audiofilter
clean_audiooutput
clean_videochroma
clean_videofilter
clean_videooutput
clean_textrenderer
clean_streamoutput
clean_crypt

