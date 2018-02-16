mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" .. -DUSE_SANDBOX=0
cmake --build . --config Release