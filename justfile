# -------------------------------------------------------------------------------------------------
# Management script
# -------------------------------------------------------------------------------------------------

default:
    @echo "No arguments supplied"
    @exit 1


# -------------------------------------------------------------------------------------------------
# Building
# -------------------------------------------------------------------------------------------------

build:
    @unset CC
    @unset CXX
    mkdir -p docs/images
    ln -sf $(pwd)/data/screenshots $(pwd)/docs/images/
    mkdir -p build
    cd build/ && CMAKE_CXX_COMPILER_LAUNCHER=ccache cmake .. -GNinja
    cd build/ && ninja

rmbuild:
    rm -rf ./build/

clang:
    export CC=/usr/bin/clang
    export CXX=/usr/bin/clang++
    just build


# -------------------------------------------------------------------------------------------------
# Cython
# -------------------------------------------------------------------------------------------------

parse_headers:
    python tools/parse_headers.py

build_cython:
    rm -rf datoviz/*.c datoviz/*.so datoviz/__pycache__ &&
    python tools/generate_cython.py &&
    python setup.py build_ext -i &&
    python setup.py develop --user


# -------------------------------------------------------------------------------------------------
# Code quality
# -------------------------------------------------------------------------------------------------

format:
    find tests/ src/ include/ -iname *.h -o -iname *.c | xargs clang-format -i

valgrind args="":
    # NOTE: need to remove -pg compiler option before running valgrind
    valgrind \
        --leak-check=full \
        --show-leak-kinds=all \
        --keep-debuginfo=yes \
        --track-origins=yes \
        --verbose \
        --suppressions=.valgrind.exceptions.txt \
        --log-file=.valgrind.out.txt \
        {{args}}

cppcheck:
    cppcheck --enable=all --inconclusive src/ include/ cli/ tests/ -i external -I include/datoviz
    # 2> .cppcheck.out.txt && \
    # echo ".cppcheck.out.txt saved"

prof:
    gprof build/datoviz gmon.out


# -------------------------------------------------------------------------------------------------
# Tests
# -------------------------------------------------------------------------------------------------

test test_name="":
    ./build/datoviz test {{test_name}}

pytest:
    DYLD_LIBRARY_PATH=$DYLD_LIBRARY_PATH:$(pwd)/build pytest datoviz/tests/ -vv


# -------------------------------------------------------------------------------------------------
# Entry-point
# -------------------------------------------------------------------------------------------------

clean:
    just rmbuild

rebuild:
    just rmbuild
    just build
