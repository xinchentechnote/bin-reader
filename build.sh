# 启用 Git 进度显示
git config --global advice.detachedHead false
git config --global core.compression 9
git config --global http.postBuffer 1048576000
git config --global url."git@github.com:".insteadOf https://github.com/
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Debug --log-level=VERBOSE -DBUILD_TESTING=ON
cmake --build build
cd build && ctest -V
cd -