git config --global url."git@github.com:".insteadOf https://github.com/
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cd build && ctest
cd -