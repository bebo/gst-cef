#!/bin/bash
set -e
CEF_BIN=cef_binary_3.3282.1726.gc8368c8_linux64.tar.bz2 # Proprietary codecs 64
# CEF_BIN=cef_binary_3.3239.1723.g071d1c1_linux64_minimal.tar.bz2 # CEF 63
# CEF_BIN=cef_binary_3.3029.1619.geeeb5d7_linux64.tar.bz2

BUILD_DIR=~/gst-cef/build
cd ~/gst-cef

rm -rf build
rm -rf package 
rm -rf ${BUILD_DIR}/dist

mkdir -vp ${BUILD_DIR}/cef
mkdir -vp ${BUILD_DIR}/dist

cd $BUILD_DIR/cef
wget https://s3-us-west-2.amazonaws.com/bebo-files/darkclouds/${CEF_BIN}
# wget http://opensource.spotify.com/cefbuilds/${CEF_BIN}
tar xvjf ${CEF_BIN} --strip-components 1
mkdir build
cd build
cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=${BUILD_DIR} ..
make -j 12
cd ${BUILD_DIR}/cef/build

cp libcef_dll_wrapper/libcef_dll_wrapper.a ${BUILD_DIR}/dist
cd ..

cp -r Release/* ${BUILD_DIR}/dist/
cp -a include/ ${BUILD_DIR}
cp -a Resources/* ${BUILD_DIR}/dist/
sudo cp /usr/local/bin/gst-launch-1.0 ${BUILD_DIR}/dist
sudo cp /usr/local/bin/gst-inspect-1.0 ${BUILD_DIR}/dist
sudo chown root:root ${BUILD_DIR}/dist/chrome-sandbox
sudo chmod 4755 ${BUILD_DIR}/dist/chrome-sandbox

cd ${BUILD_DIR}/..
./autogen.sh
./configure --prefix=${BUILD_DIR} --bindir=${BUILD_DIR}/dist --libdir=${BUILD_DIR}/dist
make -j 12

# Running a Test Pipeline
export LD_LIBRARY_PATH=${BUILD_DIR}/dist
export GST_PLUGIN_PATH=${BUILD_DIR}/dist/gstreamer-1.0
export GST_DEBUG_DUMP_DOT_DIR=/tmp/
export GST_DEBUG=3,cef:7
export DISPLAY=:0
# GST_DEBUG=cef:7 /usr/local/bin/gst-launch-1.0 cef ! videoconvert ! x264enc ! matroskamux ! fakesink 
make install
#rsync -av --progress ${BUILD_DIR}/dist/* ./package --exclude-from=.jenkins_ignore
#echo "PLEASE DO:"
#echo "sudo chown root:root ${BUILD_DIR}/dist/chrome-sandbox"
#echo "sudo chmod 4711 ${BUILD_DIR}/dist/chrome-sandbox"
#echo ""

# GST_DEBUG=cef:7 GST_PLUGIN_PATH=${HOME}/gst-cef/src/.libs /usr/local/bin/gst-launch-1.0 cef url="https://bebo.com" ! videoconvert ! x264enc ! matroskamux ! filesink location=foo.mkv
# GST_DEBUG=cef:7 GST_PLUGIN_PATH=${HOME}/mozart/virtualenv/bin /usr/local/bin/gst-launch-1.0 cef url="https://bebo.com" ! videoconvert ! x264enc ! matroskamux ! filesink location=foo.mkv
# GST_DEBUG=cef:7 /usr/local/bin/gst-launch-1.0 cef url="https://bebo.com" ! videoconvert ! x264enc ! matroskamux ! filesink location=foo.mkv
${BUILD_DIR}/dist/gst-launch-1.0 cef url="https://google.com" ! videoconvert ! x264enc ! matroskamux ! filesink location=foo.mkv
