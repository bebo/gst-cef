#!/bin/bash
set -e
CEF_BIN=cef_binary_3.3239.1723.g071d1c1_linux64.tar.bz2

BUILD_DIR=${PWD}/build

rm -rf build
rm -rf package 
rm -rf ${BUILD_DIR}/dist

mkdir -vp ${BUILD_DIR}/cef
mkdir -vp ${BUILD_DIR}/dist

cd build/cef
wget http://opensource.spotify.com/cefbuilds/${CEF_BIN}
tar xvjf ${CEF_BIN} --strip-components 1
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=${BUILD_DIR} ..
make -j 8
cd ${BUILD_DIR}/cef/build

cp libcef_dll_wrapper/libcef_dll_wrapper.a ${BUILD_DIR}/dist
cd ..

cp Release/* ${BUILD_DIR}/dist/
cp -a include/ ${BUILD_DIR}
cp -a Resources/* ${BUILD_DIR}/dist/


cd ${BUILD_DIR}/..
./autogen.sh
./configure --prefix=${BUILD_DIR} --bindir=${BUILD_DIR}/dist --libdir=${BUILD_DIR}/dist
make -j 4
make install
rsync -av --progress ${BUILD_DIR}/dist/* ./package --exclude-from=.jenkins_ignore
echo "PLEASE DO:"
echo "sudo chown root:root ${BUILD_DIR}/dist/chrome-sandbox"
echo "sudo chmod 4711 ${BUILD_DIR}/dist/chrome-sandbox"
echo ""
