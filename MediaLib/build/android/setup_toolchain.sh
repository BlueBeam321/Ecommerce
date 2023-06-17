#!/bin/sh

TOOLCHAIN=`pwd`/Toolchain
SYSROOT=$TOOLCHAIN/sysroot/
if [ -f $TOOLCHAIN/AndroidVersion.txt ]; then 
    echo 'Warning: Toolchain Already Installed';
else
    $ANDROID_NDK/build/tools/make-standalone-toolchain.sh --platform=android-16 --install-dir=$TOOLCHAIN;
fi


