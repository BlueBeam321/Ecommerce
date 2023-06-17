
source ./.env.rc

clean_libvlc() {
	echo "Cleaning libvlc..."
    cd ${LIBVLC_DIR}
    make clean
    cd ${THIS_PATH}
}

clean_libvlc

