#!/bin/bash

if [ $# -eq 0 ]
  then
    echo "No arguments supplied"
    exit
fi

if [ $1 == "build" ]
then
    mkdir -p build &&
    cd build && \
    cmake .. -GNinja && \
    DVZ_EXAMPLE= ninja && \
    cd ..
fi

if [ $1 == "gencython" ]
then
    cd bindings/cython && \
    python3 utils/gencython.py
fi

if [ $1 == "cython" ]
then
    cd bindings/cython && \
    python3 utils/gencython.py && \
    python3 setup.py build_ext -i && \
    cd ../..
fi

if [ $1 == "clang" ]
then
    mkdir -p build_clang &&
    cd build_clang && \
    CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake .. -GNinja && \
    ninja && \
    cd ..
fi

if [ $1 == "download" ]
then
    wget https://github.com/datoviz/datoviz-data/archive/master.zip -o data.zip && unzip data.zip && rm data.zip
fi

if [ $1 == "fixtest" ]
then
    mv test/screenshots/$2_fail.ppm test/screenshots/$2.ppm
fi

if [ $1 == "format" ]
then
    find examples/ test/ src/ include/ -iname *.h -o -iname *.c | xargs clang-format -i
fi

if [ $1 == "memcheck" ]
then
    valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --suppressions=.valgrind.exceptions.txt \
         --log-file=.valgrind.out.txt \
         ${@:2}
fi

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
fi

if [ $1 == "demo" ]
then
    ./build/datoviz demo $2
fi

if [ $1 == "prof" ]
then
    # valgrind --tool=callgrind --dump-instr=yes --simulate-cache=yes --collect-jumps=yes $2
    gprof build/datoviz -Al > prof.txt
fi

if [ $1 == "docker" ]
then
    docker build -t datoviz .
fi

if [ $1 == "dockerrun" ]
then
    docker run -v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=$DISPLAY -h $HOSTNAME -v $HOME/.Xauthority:/home/datoviz/.Xauthority -it datoviz
fi

if [ $1 == "doc" ]
then
    ln -sf $(pwd)/data/screenshots $(pwd)/docs/images/
    python3 utils/gendoc.py examples
    mkdocs build
fi

if [ $1 == "docs" ]
then
    python3 utils/gendoc.py examples
    mkdocs serve
fi

if [ $1 == "publish" ]
then
    cd ../datoviz.github.io/
    mkdocs gh-deploy --config-file ../datoviz/mkdocs.yml --remote-branch main
    cd ../datoviz
fi
