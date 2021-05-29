#!/bin/bash

if [ $# -eq 0 ]
  then
    echo "No arguments supplied"
    exit
fi



# -------------------------------------------------------------------------------------------------
# Building
# -------------------------------------------------------------------------------------------------

if [ $1 == "rebuild" ]
then
    rm -rf build
    mkdir -p build &&
    cd build && \
    cmake .. -GNinja && \
    ninja && \
    cd ..
fi

if [ $1 == "build" ]
then
    # Make sure macOS uses clang and not gcc
    if [[ "$OSTYPE" == "darwin"* ]]; then
        export CC=/usr/bin/clang
        export CXX=/usr/bin/clang++
    fi
    ln -sf $(pwd)/data/screenshots $(pwd)/docs/images/ &&
    mkdir -p build &&
    cd build && \
    cmake .. -GNinja && \
    ninja && \
    cd ..
fi

if [ $1 == "swiftshader" ]
then
    mkdir -p build &&
    cd build && \
    cmake .. -GNinja -DDATOVIZ_WITH_SWIFTSHADER=1 && \
    ninja && \
    cd .. && \
    LD_LIBRARY_PATH=data/swiftshader/linux ./build/datoviz test
fi



# -------------------------------------------------------------------------------------------------
# Code quality
# -------------------------------------------------------------------------------------------------

if [ $1 == "format" ]
then
    find examples/ tests/ src/ include/ -iname *.h -o -iname *.c | xargs clang-format -i
fi

if [ $1 == "valgrind" ]
then
    valgrind \
        --leak-check=full \
        --show-leak-kinds=all \
        --track-origins=yes \
        --verbose \
        --suppressions=.valgrind.exceptions.txt \
        --log-file=.valgrind.out.txt \
        ${@:2}
fi

if [ $1 == "cppcheck" ]
then
    cppcheck --enable=all --inconclusive src/ include/ cli/ tests/ -i external 2> .cppcheck.out.txt && \
    echo ".cppcheck.out.txt saved"
fi

if [ $1 == "prof" ]
then
    gprof build/datoviz gmon.out
fi



# -------------------------------------------------------------------------------------------------
# Python bindings
# -------------------------------------------------------------------------------------------------

if [ $1 == "cython" ]
then
    python3 utils/generate_cython.py && \
    cd bindings/cython && \
    python3 setup.py build_ext -i && \
    python3 setup.py develop --user && \
    cd ../..
fi

if [ $1 == "pytest" ]
then
    rm -f imgui.ini bindings/cython/imgui.ini bindings/cython/dist/imgui.ini
    pytest bindings/cython/ -vv
fi

if [ $1 == "wheel" ]
then
    ROOT_DIR=`pwd`

    # macOS
    if [[ "$OSTYPE" == "darwin"* ]]; then
        cd bindings/cython/
        python3 setup.py build_ext -i
        python3 setup.py sdist bdist_wheel
        cd dist/
        FILENAME=`ls datoviz*.whl`
        echo $FILENAME
        cp $FILENAME "$FILENAME~"
        DYLD_LIBRARY_PATH=../../../build/ delocate-wheel $FILENAME -e libvulkan -w .
        cd ../../../

    # TODO: Windows
    elif [[ "$OSTYPE" == "msys" ]]; then
        echo "TODO!"

    # manylinux
    else

        # Build the docker image.
        sudo docker build -t datoviz_wheel -f Dockerfile_wheel .

        # Clean up the Cython bindings before running the docker container.
        cd bindings/cython && \
        python3 setup.py clean --all && \
        rm -rf build dist datoviz.egg-info datoviz/*.c datoviz/*.so datoviz/__pycache__ && \
        cd ../../

        # Make the wheel and repair it.
        # Build a container based on a manylinux image, + Vulkan and other things needed by the
        # datoviz build script.
        sudo docker run --rm -v $ROOT_DIR:/io datoviz_wheel /io/wheel.sh && \
        sudo chown -R `users`:`users` bindings/cython/dist

    fi
fi

if [ $1 == "testwheel" ]
then
    # Test the wheel
    cd bindings/cython/dist
    rm -rf venv
    virtualenv venv
    venv/bin/python -m pip install --upgrade pip
    venv/bin/pip install datoviz*.whl --upgrade
    venv/bin/python -c "from datoviz import canvas, run; canvas().gui_demo(); run(0)"
    # rm -rf venv
    # cd ../../..
fi



# -------------------------------------------------------------------------------------------------
# Testing
# -------------------------------------------------------------------------------------------------

if [ $1 == "test" ]
then
    dump=""
    if [ ! -z "$3" ]
    then
        if [ $3 == "dump" ]
        then
            dump="VK_LAYER_LUNARG_api_dump"
        fi
    fi
    VK_INSTANCE_LAYERS=$dump ./build/datoviz test $2

    # When running all tests, also compile and run the standalone examples.
    if [ -z "$2" ]
    then
        cd examples/standalone/
        for filename in standalone_*.c*; do
            ./build.sh $filename automated
        done
        cd ../..
        ./build/datoviz demo
    fi
fi

if [ $1 == "demo" ]
then
    ./build/datoviz demo $2
fi



# -------------------------------------------------------------------------------------------------
# Docker
# -------------------------------------------------------------------------------------------------

if [ $1 == "docker" ]
then
    docker build -t datoviz .
fi

if [ $1 == "dockerrun" ]
then
    docker run -v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=$DISPLAY -h $HOSTNAME -v $HOME/.Xauthority:/home/datoviz/.Xauthority -it datoviz
fi



# -------------------------------------------------------------------------------------------------
# Documentation
# -------------------------------------------------------------------------------------------------

if [ $1 == "doc" ]
then
    python3 utils/generate_doc.py && \
    mkdocs build
fi

if [ $1 == "docs" ]
then
    python3 utils/generate_doc.py && \
    mkdocs serve
fi

if [ $1 == "publish" ]
then
    mkdocs build
    cd ../datoviz.github.io
    ghp-import -b main -p ../datoviz/site
    cd ../datoviz
fi



# -------------------------------------------------------------------------------------------------
# Release
# -------------------------------------------------------------------------------------------------

if [ $1 == "testpypi" ]
then
    cd bindings/cython
    twine upload --repository testpypi dist/*.whl dist/*.tar.gz
fi
