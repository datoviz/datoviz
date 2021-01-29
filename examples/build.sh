# This command requires glfw with include files and libraries
gcc standalone.c -I../include/ -I../external/cglm/include -L../build/ -lvulkan -lm -lvisky -o visky_example
./visky_example
