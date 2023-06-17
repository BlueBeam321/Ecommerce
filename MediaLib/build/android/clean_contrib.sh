
source ./.env.rc

clean_libxml2() {
	echo "Cleaning libxml2..."
    cd ${CONTRIB_DIR}/libxml2
    make clean
	make distclean
    cd ${THIS_PATH}
}

clean_libiconv() {
	echo "Cleaning libiconv..."
    cd ${CONTRIB_DIR}/libiconv
    make clean
	make distclean
    cd ${THIS_PATH}
}

clean_freetype() {
	echo "Cleaning freetype..."
    cd ${CONTRIB_DIR}/freetype
    make clean
	make distclean
    cd ${THIS_PATH}
}

clean_fontconfig() {
	echo "Cleaning fontconfig..."
    cd ${CONTRIB_DIR}/fontconfig
    make clean
	make distclean
    cd ${THIS_PATH}
}

clean_fribidi() {
	echo "Cleaning fribidi..."
    cd ${CONTRIB_DIR}/fribidi
    make clean
	make distclean
    cd ${THIS_PATH}
}

clean_libass() {
	echo "Cleaning libass..."
    cd ${CONTRIB_DIR}/libass
    make clean
	make distclean
    cd ${THIS_PATH}
}

clean_ffmpeg() {
	echo "Cleaning ffmpeg..."
    cd ${CONTRIB_DIR}/ffmpeg
    make clean
	make distclean
    cd ${THIS_PATH}
}

clean_live555() {
    echo "Cleaning live555..."
    cd ${CONTRIB_DIR}/live555
    make clean
    make distclean
    cd ${THIS_PATH}
}

clean_libdvbpsi() {
    echo "Cleaning libdvbpsi..."
    cd ${CONTRIB_DIR}/libdvbpsi
    make clean
    make distclean
    cd ${THIS_PATH}
}

clean_libebml() {
    echo "Cleaning libebml..."
    cd ${INSTALL_TMP_DIR}/libebml
    make clean
    cd ${THIS_PATH}
    rm ${INSTALL_TMP_DIR}/libebml -rf
    rm ${CONTRIB_DIR}/libebml/libs -rf
}

clean_libmatroska() {
    echo "Cleaning libmatroska..."
    cd ${INSTALL_TMP_DIR}/libmatroska
    make clean
    cd ${THIS_PATH}
    rm ${INSTALL_TMP_DIR}/libmatroska -rf
    rm ${CONTRIB_DIR}/libmatroska/libs -rf
}

clean_x264() {
	echo "Cleaning x264..."
    cd ${CONTRIB_DIR}/x264
    make clean
	make distclean
    cd ${THIS_PATH}
}

clean_libxml2
clean_libiconv
clean_freetype
clean_fontconfig
clean_fribidi
clean_libass
clean_ffmpeg
clean_live555
clean_libdvbpsi
clean_libebml
clean_libmatroska
clean_x264

