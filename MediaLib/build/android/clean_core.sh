
source ./.env.rc

clean_compat() {
	echo "Cleaning compat..."
    cd ${CORE_DIR}/compat
    make clean
    cd ${THIS_PATH}
}

clean_misc() {
	echo "Cleaning misc..."
    cd ${CORE_DIR}/misc
    make clean
    cd ${THIS_PATH}
}

clean_videooutput() {
	echo "Cleaning videooutput..."
    cd ${CORE_DIR}/video_output
    make clean
    cd ${THIS_PATH}
}

clean_text() {
	echo "Cleaning text..."
    cd ${CORE_DIR}/text
    make clean
    cd ${THIS_PATH}
}

clean_input() {
	echo "Cleaning input..."
    cd ${CORE_DIR}/input
    make clean
    cd ${THIS_PATH}
}

clean_modules() {
	echo "Cleaning modules..."
    cd ${CORE_DIR}/modules
    make clean
    cd ${THIS_PATH}
}

clean_config() {
	echo "Cleaning config..."
    cd ${CORE_DIR}/config
    make clean
    cd ${THIS_PATH}
}

clean_network() {
	echo "Cleaning network..."
    cd ${CORE_DIR}/network
    make clean
    cd ${THIS_PATH}
}

clean_playlist() {
	echo "Cleaning playlist..."
    cd ${CORE_DIR}/playlist
    make clean
    cd ${THIS_PATH}
}

clean_audiooutput() {
	echo "Cleaning audiooutput..."
    cd ${CORE_DIR}/audio_output
    make clean
    cd ${THIS_PATH}
}

clean_streamoutput() {
	echo "Cleaning streamoutput..."
    cd ${CORE_DIR}/stream_output
    make clean
    cd ${THIS_PATH}
}

clean_compat
clean_misc
clean_videooutput
clean_text
clean_input
clean_modules
clean_config
clean_network
clean_playlist
clean_audiooutput
clean_streamoutput

