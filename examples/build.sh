# This command requires glfw with include files and libraries
# NOTE: use -lgfw3 on macOS
gcc standalone.c -I../include/ -I../external/cglm/include -I../build/_deps/glfw-src/include \
    -L../build/ -L../build/_deps/glfw-build/src -lvulkan -lm -lglfw -lvisky -o visky_example
#LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../build ./visky_example
