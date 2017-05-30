

# CEF 


sudo apt install build-essential libgtk2.0-dev libgtkglext1-dev

http://opensource.spotify.com/cefbuilds/index.html
http://opensource.spotify.com/cefbuilds/cef_binary_3.3029.1619.geeeb5d7_linux64.tar.bz2
tar xvjf cef_binary_3.3029.1619.geeeb5d7_linux64.tar.bz2
 cd cef_binary_3.3029.1619.geeeb5d7_linux64
 mkdir build
 cd build

 cmake -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr/local/cef ..

sudo mkdir -p /usr/local/cef/lib
sudo mkdir -p /usr/local/cef/include

sudo cp libcef_dll_wrapper/libcef_dll_wrapper.a /usr/local/cef/lib/
sudo cp ../Release/* /usr/local/cef/lib
sudo cp -a ../include/ /usr/local/cef/
sudo cp -a ../Resources/* /usr/local/cef/lib/
sudo chown root:root /usr/local/cef/lib/chrome-sandbox
sudo chmod 4711 /usr/local/cef/lib/chrome-sandbox


# for right now:

cd gst-cef/src
sudo cp -a /usr/local/cef/lib/* src/




# gst-cef 
./autogen.sh

## to rebuild do
make

# to run:
cd src
export LD_LIBRARY_PATH=/usr/local/cef/lib
export DISPLAY=:0
Xvfb :0 -screen 0 1280x720x16 &
GST_DEBUG=cef:7 GST_PLUGIN_PATH=${HOME}/gst-cef/src/.libs /usr/local/bin/gst-launch-1.0 cef ! videoconvert ! x264enc ! matroskamux ! filesink location=foo.mkv

