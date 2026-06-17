cmake -S "%SRC_DIR%" -B build-conda -G Ninja ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%PREFIX%\\Library" ^
    -DDVZ_BUILD_TESTING=OFF ^
    -DDVZ_BUILD_EXAMPLES=OFF ^
    -DDVZ_INSTALL=ON ^
    -DDVZ_VENDORED_DEPS=OFF ^
    -DDVZ_CGLM_SOURCE=SYSTEM ^
    -DDVZ_MIMALLOC_SOURCE=SYSTEM ^
    -DDVZ_KVAZAAR_SOURCE=OFF ^
    -DDVZ_ENABLE_CUDA=OFF ^
    -DDVZ_ENABLE_QT_BRIDGE=OFF
if errorlevel 1 exit 1

cmake --build build-conda --target install
if errorlevel 1 exit 1
