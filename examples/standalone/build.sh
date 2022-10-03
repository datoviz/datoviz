# This command requires glfw with include files and libraries.

if [ -z "$1" ]; then 
export DVZ_EXAMPLE_FILE="standalone_canvas.c";
else
export DVZ_EXAMPLE_FILE=$1
fi

# This build script should be improved, use cmake perhaps
#export DVZ_EXAMPLE_FILE=$1
export DVZ_ROOT=../../
export AUTOMATED=""
if [ ! -z "$2" ]
then
    AUTOMATED="-DAUTOMATED=1"
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
    GLFW="-lglfw3"
else
    GLFW="-lglfw"
fi

# Compile the example.
# NOTE: use -lgfw3 on macOS
gcc $DVZ_EXAMPLE_FILE \
    $AUTOMATED \
    -I$DVZ_ROOT/include/ \
    -I$DVZ_ROOT/build/_deps/cglm-src/include \
    -I$DVZ_ROOT/build/_deps/glfw-src/include \
    -I$DVZ_ROOT/external/imgui/ \
    -I$DVZ_ROOT/external/ \
    -I$VULKAN_SDK/include/ \
    -L$VULKAN_SDK/lib/ \
    -L$DVZ_ROOT/build/ \
    -L$DVZ_ROOT/build/_deps/glfw-build/src \
    -lvulkan -lm -ldatoviz -o datoviz_example

# Compile the shaders.
for filename in *.vert *.frag; do
    glslc $filename -o "$filename.spv" -I$DVZ_ROOT/include/datoviz/glsl
done

# NOTE: libdatoviz must be in the linker path before running the example (dynamic linking)
# LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$DVZ_ROOT/build ./datoviz_example
