# This command requires glfw with include files and libraries
# NOTE: use -lgfw3 on macOS
# This build script should be improved, use cmake perhaps
export DVZ_EXAMPLE_FILE=$1
gcc $DVZ_EXAMPLE_FILE -I../include/ -I../external/cglm/include -I../build/_deps/glfw-src/include \
    -I../external/imgui/ -I../external/ \
    -L../build/ -L../build/_deps/glfw-build/src -lvulkan -lm -lglfw -ldatoviz -o datoviz_example

glslc custom_point.vert -o custom_point.vert.spv -I../include/datoviz/glsl
glslc custom_point.frag -o custom_point.frag.spv -I../include/datoviz/glsl

glslc triangle.vert -o triangle.vert.spv -I../include/datoviz/glsl
glslc triangle.frag -o triangle.frag.spv -I../include/datoviz/glsl

# NOTE: libdatoviz must be in the linker path before running the example (dynamic linking)
LD_LIBRARY_PATH=$LD_LIBRARY_PATH:../build ./datoviz_example
