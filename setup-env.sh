# Linux
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(pwd)/build
# macOS
export DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$(pwd)/build

# Workaround Anaconda-related issue: https://github.com/datoviz/datoviz/issues/8
export LD_PRELOAD=/lib/x86_64-linux-gnu/libcairo.so.2
