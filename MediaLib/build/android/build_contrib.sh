#!/bin/bash -e

source ./.env.rc

build_libxml2() {
    echo "Building libxml2..."
    
    cd ${CONTRIB_DIR}/libxml2
	./configure --prefix=${INSTALL_DIR} \
		--host=${CROSS_COMPILE} \
		--enable-static \
		--disable-shared \
		--without-python \
		--without-lzma \
		--without-zlib
    make
    make install
    cd ${THIS_PATH}
}

build_libiconv() {
    echo "Building libiconv..."
    cd ${CONTRIB_DIR}/libiconv
    ./configure --prefix=${INSTALL_DIR} \
		--host=${CROSS_COMPILE} \
		--enable-static \
		--disable-shared
    make
    make install
    cd ${THIS_PATH}
}

build_freetype() {
    echo "Building freetype..."
    cd ${CONTRIB_DIR}/freetype
    ./configure --prefix=${INSTALL_DIR} \
		--host=${CROSS_COMPILE} \
		--enable-static \
		--disable-shared \
		--without-zlib
    make
    make install
    cd ${THIS_PATH}
}

build_fontconfig() {
    echo "Building fontconfig..."
    cd ${CONTRIB_DIR}/fontconfig
    ./configure --prefix=${INSTALL_DIR} \
		--host=${CROSS_COMPILE} \
		--enable-static \
		--disable-shared \
		--disable-docs \
		--enable-libxml2 \
		PKG_CONFIG_LIBDIR=${INSTALL_DIR}/lib/pkgconfig
    make
    make install
    cd ${THIS_PATH}
}

build_fribidi() {
    echo "Building fribidi..."
    cd ${CONTRIB_DIR}/fribidi
    ./configure --prefix=${INSTALL_DIR} \
		--host=${CROSS_COMPILE} \
		--enable-static \
		--disable-shared
    make
    make install
    cd ${THIS_PATH}
}

build_libass() {
    echo "Building libass..."
    cd ${CONTRIB_DIR}/libass
    ./configure --prefix=${INSTALL_DIR} \
		--host=${CROSS_COMPILE} \
		--enable-static \
		--disable-shared \
		PKG_CONFIG_LIBDIR=${INSTALL_DIR}/lib/pkgconfig
    make
    make install
    cd ${THIS_PATH}
}

build_ffmpeg() {
    echo "Building ffmpeg..."
    cd ${CONTRIB_DIR}/ffmpeg
	
	./configure --prefix=${INSTALL_DIR} \
		--target-os=android \
		--arch=arm \
		--enable-cross-compile \
		--cross-prefix=${CROSS_COMPILE}- \
		--enable-static \
		--disable-shared \
		--disable-symver \
		--disable-encoders \
		--disable-avdevice \
		--disable-swresample \
		--disable-postproc \
		--disable-avfilter \
		--disable-muxers \
		--enable-pthreads \
		--enable-hwaccel=h264_vaapi \
		--enable-hwaccel=h264_vaapi \
		--enable-hwaccel=h264_dxva2 \
		--enable-hwaccel=mpeg4_vaapi \
		--enable-parsers \
		--enable-hwaccels \
		--disable-zlib \
		--disable-xlib \
		--disable-doc \
		--disable-ffplay \
		--disable-ffmpeg \
		--disable-ffplay \
		--disable-ffprobe \
		--enable-nonfree \
		--enable-version3 \
		--enable-asm \
		--enable-jni \
		--enable-mediacodec \
		--extra-cflags="${CFLAGS} ${EXTRA_CFLAGS}" \
		--extra-ldflags="${EXTRA_LDFLAGS}"	
    make
    make install
    cd ${THIS_PATH}
}

build_live555() {
    echo "Building live555..."
    cd ${CONTRIB_DIR}/live555
    ./genMakefiles android
    make
    make install
    cd ${THIS_PATH}
}

build_libdvbpsi() {
    echo "Building libdvbpsi..."
    cd ${CONTRIB_DIR}/libdvbpsi
    ./configure --prefix=${INSTALL_DIR} \
		--host=${CROSS_COMPILE} \
		--enable-static \
		--disable-shared
    make
    make install
    cd ${THIS_PATH}
}

build_libebml() {
    echo "Building libebml..."
    mkdir -p ${INSTALL_TMP_DIR}/libebml
    cd ${INSTALL_TMP_DIR}/libebml
    cmake -DCMAKE_TOOLCHAIN_FILE=${THIS_PATH}/makefiles/android.toolchain.cmake \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
        ${CONTRIB_DIR}/libebml 
    make
    make install
    cd ${THIS_PATH}
}

build_libmatroska() {
    echo "Building libmatroska..."
    mkdir -p ${INSTALL_TMP_DIR}/libmatroska
    cd ${INSTALL_TMP_DIR}/libmatroska
    cmake -DCMAKE_TOOLCHAIN_FILE=${THIS_PATH}/makefiles/android.toolchain.cmake \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
        ${CONTRIB_DIR}/libmatroska 
    make
    make install
    cd ${THIS_PATH}
}

build_x264() {
    echo "Building x264..."
    cd ${CONTRIB_DIR}/x264
    ./configure --prefix=${INSTALL_DIR} \
		--host=${CROSS_COMPILE} \
		--cross-prefix=${CROSS_COMPILE}- \
		--enable-static \
		--disable-asm \
		--disable-avs \
		--disable-swscale \
		--disable-lavf \
		--disable-ffms \
		--disable-gpac \
		--disable-lsmash \
		--disable-cli \
		--disable-opencl
    make
    make install
    cd ${THIS_PATH}
}

build_libxml2
build_libiconv
build_freetype
build_fontconfig
build_fribidi
build_libass
build_ffmpeg
build_live555
build_libdvbpsi
build_libebml
build_libmatroska
build_x264

