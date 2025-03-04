# -------------------------------------------------------------------------------------------------
# CMake build script
# -------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.10)
set(CMAKE_MESSAGE_LOG_LEVEL INFO)
enable_testing()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include("cmake/colormsg.cmake")
include(FetchContent)
include(ExternalProject)

# -------------------------------------------------------------------------------------------------
# Extract version number from include/datoviz_version.h
# -------------------------------------------------------------------------------------------------

# Read the version header file
set(VERSION_HEADER_FILE "${CMAKE_SOURCE_DIR}/include/datoviz_version.h")
file(READ "${VERSION_HEADER_FILE}" VERSION_CONTENTS)

# Extract major version
string(REGEX MATCH "#define DVZ_VERSION_MAJOR ([0-9]+)" _ ${VERSION_CONTENTS})
set(DVZ_VERSION_MAJOR ${CMAKE_MATCH_1})

# Extract minor version
string(REGEX MATCH "#define DVZ_VERSION_MINOR ([0-9]+)" _ ${VERSION_CONTENTS})
set(DVZ_VERSION_MINOR ${CMAKE_MATCH_1})

# Extract patch version
string(REGEX MATCH "#define DVZ_VERSION_PATCH ([0-9]+)" _ ${VERSION_CONTENTS})
set(DVZ_VERSION_PATCH ${CMAKE_MATCH_1})

# Optionally, you can set the project version
set(DVZ_VERSION "${DVZ_VERSION_MAJOR}.${DVZ_VERSION_MINOR}.${DVZ_VERSION_PATCH}")

# -------------------------------------------------------------------------------------------------
# Datoviz project
# -------------------------------------------------------------------------------------------------
project(datoviz VERSION ${DVZ_VERSION} DESCRIPTION "datoviz")

# -------------------------------------------------------------------------------------------------
# Datoviz options
# -------------------------------------------------------------------------------------------------
option(DATOVIZ_WITH_CLI "Build Datoviz command-line interface with tests and demos" ON)
option(DATOVIZ_WITH_ZLIB "Build Datoviz with zlib" ON) # for gzip decompression for test data
option(DATOVIZ_WITH_GLFW "Build Datoviz with glfw" ON)
option(DATOVIZ_WITH_MSDF "Build Datoviz with msdfgen" ON)
option(DATOVIZ_WITH_SHADERC "Build Datoviz with shaderc" OFF)
option(DATOVIZ_WITH_OPENMP "Build Datoviz with OpenMP support" OFF) # disabling for now
option(DATOVIZ_WITH_QT "Build Datoviz with Qt6" OFF)

# option(DATOVIZ_WITH_SWIFTSHADER "Build Datoviz with Swiftshader support" OFF)

# -------------------------------------------------------------------------------------------------
# Build type
# -------------------------------------------------------------------------------------------------

# set(CMAKE_BUILD_TYPE Debug)
message("Build type: ${CMAKE_BUILD_TYPE}")
set(CMAKE_C_STANDARD)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# NOTE: -one-off script to remove unnecessary #includes
# need to install llvm, llvm-dev, clang from the same version of iwyu
# fix the source files with:
#
# 1. In the build/ subfolder of datoviz:
# cmake ..
# make C=include-what-you-use CFLAGS="-Xiwyu --error_always" 2>iwyu.out
#
# 2. In the iwyu git cloned directory:
# python fix_includes.py -n --nosafe_headers
# --ignore_re="_deps|_shaders.c|_textures.c|_fonts.c"
# < ~/git/datoviz-distributed/build/iwyu.out
#
# find_program(iwyu_path NAMES include-what-you-use iwyu REQUIRED)
# set(CMAKE_C_INCLUDE_WHAT_YOU_USE iwyu -Xiwyu;any;-Xiwyu;iwyu;-Xiwyu;args)

# -------------------------------------------------------------------------------------------------
# Detect the OS
# -------------------------------------------------------------------------------------------------
set(OS_LINUX 0)
set(OS_MACOS 0)
set(OS_WINDOWS 0)

if(UNIX AND NOT APPLE)
    set(OS_LINUX 1)
elseif(APPLE)
    set(OS_MACOS 1)
elseif(WIN32)
    set(OS_WINDOWS 1)
endif()

# -------------------------------------------------------------------------------------------------
# Detect the compiler
# -------------------------------------------------------------------------------------------------
set(CC_GCC 0)
set(CC_CLANG 0)
set(CC_MSVC 0)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # message("compiler is GCC")
    set(CC_GCC 1)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # message("compiler is clang")
    set(CC_CLANG 1)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # message("compiler is MSVC")
    set(CC_MSVC 1)
endif()

# -------------------------------------------------------------------------------------------------
# Directories
# -------------------------------------------------------------------------------------------------
set(DATA_DIR "${CMAKE_SOURCE_DIR}/data")
set(SPIRV_DIR ${CMAKE_BINARY_DIR}/spirv)
set(ARTIFACTS_DIR "${CMAKE_BINARY_DIR}/artifacts")

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR})
file(MAKE_DIRECTORY ${SPIRV_DIR})
file(MAKE_DIRECTORY "${ARTIFACTS_DIR}")

# -------------------------------------------------------------------------------------------------
# Include directories
# -------------------------------------------------------------------------------------------------
set(INCL_DIRS
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/datoviz
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/imgui
)

# ---- OS-specific directories --------------------------------------------------------------------
if(${OS_MACOS})
    set(INCL_DIRS ${INCL_DIRS} /usr/local/include)
elseif(${OS_WINDOWS})
    set(INCL_DIRS ${INCL_DIRS}) # $ENV{VULKAN_SDK}/include $ENV{CGLM_INCLUDE})
    find_path(MINGW_DIR NAMES "gcc.exe")
endif()

# -------------------------------------------------------------------------------------------------
# Linking libraries
# -------------------------------------------------------------------------------------------------
# Link with system libraries.
if(NOT ${WIN32})
    set(LINK_LIBS m dl pthread ${LINK_LIBS})
endif()

set(CORE_LIBS ${LINK_LIBS})
set(SCENE_LIBS ${LINK_LIBS})

# ---- OS-specific linking libraries --------------------------------------------------------------
if(${OS_MACOS})
    set(CMAKE_OSX_DEPLOYMENT_TARGET 12)
    link_directories(/usr/local/lib)
endif()

# -------------------------------------------------------------------------------------------------
# Build dependencies
# -------------------------------------------------------------------------------------------------

# NOTE: if ON, will avoid fetching git repositories, thereby making builds faster.
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

# ---- cglm ---------------------------------------------------------------------------------------
option(CGLM_STATIC "Static build" ON)
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm/
    GIT_TAG v0.9.4
)
FetchContent_MakeAvailable(cglm)
set(INCL_DIRS ${INCL_DIRS} ${cglm_SOURCE_DIR}/include)

# ---- zlib ---------------------------------------------------------------------------------------
set(HAS_ZLIB 0)

if(DATOVIZ_WITH_ZLIB)
    find_package(ZLIB)

    if(ZLIB_FOUND)
        set(CORE_LIBS ${CORE_LIBS} ZLIB::ZLIB)
        set(HAS_ZLIB 1)
    endif()
endif()

# ---- qt ---------------------------------------------------------------------------------------
set(HAS_QT 0)

if(NOT ${OS_LINUX})
    set(DATOVIZ_WITH_QT 0)
    message(WARNING "Disabling Qt build on macOS and Windows for now")
endif()

if(DATOVIZ_WITH_QT)
    set(CMAKE_MESSAGE_LOG_LEVEL WARNING)
    find_package(Qt6 COMPONENTS Core Gui Widgets)
    set(CMAKE_MESSAGE_LOG_LEVEL DEBUG)

    if(Qt6_FOUND)
        set(INCL_DIRS ${INCL_DIRS}
            ${Qt6Core_INCLUDE_DIRS}
            ${Qt6Gui_INCLUDE_DIRS}
            ${Qt6Widgets_INCLUDE_DIRS})
        set(CORE_LIBS ${CORE_LIBS} Qt6::Core Qt6::Gui Qt6::Widgets)
        set(HAS_QT 1)
    endif()
endif()

# ---- openmp -------------------------------------------------------------------------------------
set(HAS_OPENMP 0)

if(${OS_WINDOWS})
    set(DATOVIZ_WITH_OPENMP OFF)

    message(WARNING
        "Disabling OpenMP build on Windows for now due to weird bugs (it leads to a .dll that "
        "is not loadable in ctypes on Windows)")
endif()

if(DATOVIZ_WITH_OPENMP)
    find_package(OpenMP)

    if(OPENMP_FOUND)
        message(STATUS "OpenMP found. Adding support.")
        set(CORE_LIBS OpenMP::OpenMP_CXX ${CORE_LIBS})
        set(HAS_OPENMP 1)
    else()
        message(WARNING "OpenMP not found. Disabling OpenMP support.")
    endif()
endif()

# ---- glfw ---------------------------------------------------------------------------------------
set(HAS_GLFW 0)

if(DATOVIZ_WITH_GLFW)
    set(HAS_GLFW 1)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw/
        GIT_TAG 3.4
    )
    FetchContent_MakeAvailable(glfw)

    set(INCL_DIRS ${INCL_DIRS} "${glfw_SOURCE_DIR}/include")
    set(CORE_LIBS ${CORE_LIBS} glfw)
endif()

# ---- Vulkan -------------------------------------------------------------------------------------
if(${OS_MACOS})
    set(GLSLC_PATH "${CMAKE_SOURCE_DIR}/bin/vulkan/macos")
    set(VULKAN_DIR "${CMAKE_SOURCE_DIR}/libs/vulkan/macos")
    set(VULKAN_LIB "${VULKAN_DIR}/libvulkan.dylib")

    find_program(GLSLC "glslc" HINTS "${GLSLC_PATH}" "$ENV{VULKAN_SDK}/bin" REQUIRED)
    set(CORE_LIBS ${CORE_LIBS} "${VULKAN_LIB}")
elseif(${OS_WINDOWS})
    # set(GLSLC_PATH "${CMAKE_SOURCE_DIR}/bin/vulkan/windows")
    # set(VULKAN_DIR "${CMAKE_SOURCE_DIR}/libs/vulkan/windows")
    # set(VULKAN_LIB "${VULKAN_DIR}/vulkan-1.dll")
    find_package(Vulkan REQUIRED)
    set(CORE_LIBS ${CORE_LIBS} ${Vulkan_LIBRARIES})
    find_program(GLSLC "glslc" HINTS "$ENV{VULKAN_SDK}\\bin" REQUIRED)
else()
    set(GLSLC_PATH "${CMAKE_SOURCE_DIR}/bin/vulkan/linux")
    set(VULKAN_DIR "${CMAKE_SOURCE_DIR}/libs/vulkan/linux")
    set(VULKAN_LIB "${VULKAN_DIR}/libvulkan.so")

    # HACK for manylinux almalinux 8
    if(NOT EXISTS "${VULKAN_LIB}")
        set(VULKAN_LIB "/usr/lib64/libvulkan.so.1")
    endif()

    find_program(GLSLC "glslc" HINTS "${GLSLC_PATH}" "$ENV{VULKAN_SDK}/bin" REQUIRED)
    set(CORE_LIBS ${CORE_LIBS} "${VULKAN_LIB}")
endif()

# ---- shaderc ------------------------------------------------------------------------------------
set(HAS_SHADERC 0)

if(DATOVIZ_WITH_SHADERC)
    if(${OS_MACOS})
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
            set(CORE_LIBS ${CORE_LIBS} "${CMAKE_SOURCE_DIR}/libs/shaderc/macos_x86_64/libshaderc_shared.dylib")
        else()
            set(CORE_LIBS ${CORE_LIBS} "${CMAKE_SOURCE_DIR}/libs/shaderc/macos_arm64/libshaderc_shared.dylib")
        endif()
    elseif(${OS_WINDOWS})
        set(CORE_LIBS ${CORE_LIBS} "${CMAKE_SOURCE_DIR}/libs/shaderc/windows/libshaderc_shared.dll")
    else()
        set(CORE_LIBS ${CORE_LIBS} "${CMAKE_SOURCE_DIR}/libs/shaderc/linux/libshaderc_shared.so")
    endif()

    # set(INCL_DIRS ${INCL_DIRS} "${shaderc_SOURCE_DIR}/libshaderc/include")

    # # SPIRV-Headers
    # set(SPIRV_HEADERS_ENABLE_TESTS OFF)
    # set(SPIRV_HEADERS_ENABLE_INSTALL OFF)
    # FetchContent_Declare(
    # SPIRV-Headers
    # GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Headers.git
    # GIT_TAG main
    # )
    # FetchContent_MakeAvailable(SPIRV-Headers)

    # # SPIRV-Tools
    # set(SPIRV_SKIP_TESTS ON)
    # set(SPIRV_SKIP_EXECUTABLES ON)
    # set(SPIRV_TOOLS_BUILD_STATIC OFF)
    # FetchContent_Declare(
    # SPIRV-Tools
    # GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Tools.git
    # GIT_TAG main
    # )
    # FetchContent_MakeAvailable(SPIRV-Tools)

    # # glslang
    # set(GLSLANG_TESTS OFF)
    # set(GLSLANG_ENABLE_INSTALL OFF)
    # set(ENABLE_GLSLANG_BINARIES OFF)
    # set(BUILD_SHARED_LIBS OFF)

    # # set(ENABLE_HLSL ON)
    # FetchContent_Declare(
    # glslang
    # GIT_REPOSITORY https://github.com/KhronosGroup/glslang
    # GIT_TAG main
    # )
    # FetchContent_MakeAvailable(glslang)

    # # shaderc
    # set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
    # set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
    # set(SHADERC_SKIP_INSTALL ON CACHE BOOL "" FORCE)
    # FetchContent_Declare(
    # shaderc
    # GIT_REPOSITORY https://github.com/google/shaderc
    # GIT_TAG main
    # )
    # FetchContent_MakeAvailable(shaderc)

    # set(INCL_DIRS ${INCL_DIRS} "${shaderc_SOURCE_DIR}/libshaderc/include")
    # set(CORE_LIBS ${CORE_LIBS} shaderc)
    set(HAS_SHADERC 1)
endif()

# ---- msdf ------------------------------------------------------------------------------------
set(HAS_MSDF 0)

if(DATOVIZ_WITH_MSDF)
    set(HAS_MSDF 1)

    set(MSDFGEN_CORE_ONLY OFF)
    set(MSDFGEN_BUILD_STANDALONE OFF)
    set(MSDFGEN_USE_SKIA OFF)
    set(MSDFGEN_DISABLE_SVG OFF)

    set(MSDF_ATLAS_BUILD_STANDALONE OFF)
    set(MSDF_ATLAS_USE_VCPKG ${OS_WINDOWS})
    set(MSDF_ATLAS_USE_SKIA OFF)
    set(MSDF_ATLAS_DYNAMIC_RUNTIME OFF)
    set(MSDF_ATLAS_MSDFGEN_EXTERNAL OFF)
    set(MSDF_ATLAS_INSTALL OFF)

    set(BUILD_SHARED_LIBS OFF)

    # Freetype is required for msdf-atlas-gen
    # sudo apt install libtinyxml2-dev libfreetype-dev
    find_package(PNG REQUIRED)
    find_package(Freetype REQUIRED)

    # tinyxml2
    set(tinyxml2_BUILD_TESTING OFF)
    FetchContent_Declare(
        tinyxml2
        GIT_REPOSITORY https://github.com/leethomason/tinyxml2.git
        GIT_TAG 10.0.0
    )
    FetchContent_MakeAvailable(tinyxml2)

    set_target_properties(tinyxml2 PROPERTIES POSITION_INDEPENDENT_CODE ON)

    # NOTE: this fixes a build problem on macOS with cstddef.h <stddef> where the import
    # below is added to the compilation command and should be removed for the build to succeed:
    # -isystem /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX15.2.sdk/usr/include
    if(${OS_MACOS})
        set(CMAKE_NO_SYSTEM_FROM_IMPORTED ON)
    endif()

    # MSDF atlas
    FetchContent_Declare(
        msdfgen-atlas
        GIT_REPOSITORY https://github.com/Chlumsky/msdf-atlas-gen.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(msdfgen-atlas)

    # add_library(msdfgen_core STATIC IMPORTED)
    # set_target_properties(msdfgen_core PROPERTIES
    # IMPORTED_LOCATION "${msdfgen-atlas_SOURCE_DIR}/msdfgen/build/libmsdfgen-core.a"
    # POSITION_INDEPENDENT_CODE ON
    # )
    set(INCL_DIRS ${INCL_DIRS}
        ${msdfgen_SOURCE_DIR}
        ${msdfgen-atlas_SOURCE_DIR}
        ${msdfgen-atlas_SOURCE_DIR}/msdfgen
        ${msdfgen-atlas_SOURCE_DIR}/msdf-atlas-gen
        ${FREETYPE_INCLUDE_DIR_ft2build}
    )
    set(SCENE_LIBS ${SCENE_LIBS} msdf-atlas-gen::msdf-atlas-gen ${FREETYPE_LIBRARIES})
endif()

# -------------------------------------------------------------------------------------------------
# Compile definitions
# -------------------------------------------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENABLE_VALIDATION_LAYERS 1)
else()
    set(ENABLE_VALIDATION_LAYERS 0)
endif()

set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS}
    LOG_USE_COLOR
    ENABLE_VALIDATION_LAYERS=${ENABLE_VALIDATION_LAYERS}
    DEBUG=$<IF:$<CONFIG:Debug>,1,0>

    # Directories.
    ROOT_DIR=\"${CMAKE_SOURCE_DIR}\"
    DATA_DIR=\"${DATA_DIR}\"
    SPIRV_DIR=\"${SPIRV_DIR}\"
    ARTIFACTS_DIR=\"${ARTIFACTS_DIR}\"

    # OS.
    OS_LINUX=${OS_LINUX}
    OS_MACOS=${OS_MACOS}
    OS_WINDOWS=${OS_WINDOWS}

    # Compiler.
    CC_MSVC=${CC_MSVC}
    CC_GCC=${CC_GCC}
    CC_CLANG=${CC_CLANG}

    # Dependencies.
    HAS_OPENMP=${HAS_OPENMP}
    HAS_ZLIB=${HAS_ZLIB}
    HAS_GLFW=${HAS_GLFW}
    HAS_MSDF=${HAS_MSDF}
    HAS_SHADERC=${HAS_SHADERC}
    HAS_QT=${HAS_QT}

    # HACK: this apparently fixes the msdfgen-config.h not found bug
    MSDFGEN_PUBLIC=
)

# -------------------------------------------------------------------------------------------------
# Compiler options
# -------------------------------------------------------------------------------------------------

# ---- common compiler options --------------------------------------------------------------------
set(COMMON_FLAGS "-g -m64 -pedantic \
    -Wall \
    -Wextra \
    -Werror=vla \
    -Wcast-align \
    -Wcast-qual \
    -Wredundant-decls \
    -Wswitch-default \
    -Wmissing-declarations \
    -Wmissing-include-dirs \
    -Wno-unused-result \
    -Wno-unused-function \
    -Wno-unused-variable \
    -Wno-unused-parameter \
    -Wno-format-nonliteral \
    -Wno-strict-overflow \
    -Wsign-conversion \
    -Wshadow \
    -Wformat=2 \
    -Winit-self \
    -Wundef \
    ")

# Optimization level.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(COMMON_FLAGS "${COMMON_FLAGS} -O0 ")
else()
    set(COMMON_FLAGS "${COMMON_FLAGS} -O2 ")
endif()

# -Wdisabled-optimization \
# -Wno-missing-field-initializers
# -Wno-variadic-macros")

# ---- compiler-specific options ------------------------------------------------------------------
if(CC_GCC)
    # gcc

    # # for fpng
    # set(GCC_COMPILE_FLAGS "\
    # -fvisibility=hidden
    # -fPIC \
    # -fno-strict-aliasing \
    # -fno-omit-frame-pointer \
    # -D_LARGEFILE64_SOURCE=1 \
    # -D_FILE_OFFSET_BITS=64 \
    # -Wall \
    # -Wextra \
    # -Isrc")
    set(COMMON_FLAGS "${COMMON_FLAGS} \
        -Wlogical-op \
        -Wno-stringop-overread \
        -fvisibility=hidden \
        -fdiagnostics-color=always \
        -Wno-unused-but-set-variable \
        -DFPNG_NO_SSE=0 -msse4.1 -mpclmul") # for fpng

    if(HAS_OPENMP)
        set(COMMON_FLAGS "${COMMON_FLAGS} ${OpenMP_CXX_FLAGS}")
    endif()

    # NOTES: static analysis for later?
    # -fanalyzer \
    # -Wno-analyzer-possible-null-dereference \
    # -Wno-analyzer-possible-null-argument \
    # -Wno-analyzer-malloc-leak \
    # -Wno-analyzer-use-after-free \
    # -Wno-analyzer-double-free \

    # -Wdouble-promotion \
    # fsanitize=undefined,address

    # NOTE: to comment when using valgrind
    # Also remove in RELEASE mode?
    # if(NOT OS_WINDOWS)
    # set(COMMON_FLAGS "${COMMON_FLAGS} -pg")
    # endif()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} \
        -std=gnu11 -Wmissing-prototypes -Wimplicit-function-declaration -Wstrict-prototypes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -std=c++17")

elseif(CC_CLANG)
    # clang
    # on Ubuntu, must do: sudo apt-get install libc++abi-dev libc++-dev
    set(COMMON_FLAGS "${COMMON_FLAGS} \
        -Wno-unused-parameter \
        -fcolor-diagnostics \
        -Wno-missing-braces \
        -Wmissing-prototypes \
        ")

    # -fcatch-undefined-behavior \  # runtime checks

    # for fpng
    set(COMMON_FLAGS "${COMMON_FLAGS} -fdiagnostics-color=always")

    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(COMMON_FLAGS "${COMMON_FLAGS} -DFPNG_NO_SSE=0 -msse4.1 -mpclmul")
    else()
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -std=gnu11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -stdlib=libc++ -std=c++17")
    set(CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS} ${COMMON_FLAGS}
        -Wno-partial-availability -Wno-unguarded-availability-new")

elseif(CC_MSVC)
    # MSVC
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif()

# -------------------------------------------------------------------------------------------------
# Shaders compilation
# -------------------------------------------------------------------------------------------------
file(GLOB shader_sources
    "src/scene/glsl/*.*"
    "tests/glsl/*.*"
)

foreach(shader_source ${shader_sources})
    get_filename_component(shader_name ${shader_source} NAME)

    # NOTE: there was a bug because the shader output was a relative path, not a full path
    # and cmake believed the output was not properly created by the custom command glslc.
    set(shader_output "${SPIRV_DIR}/${shader_name}.spv")
    add_custom_command(
        OUTPUT ${shader_output}
        COMMAND ${GLSLC}
        -o "${shader_output}" ${shader_source}
        -I "${CMAKE_SOURCE_DIR}/include/datoviz/scene/glsl"
        DEPENDS ${shader_source}
        IMPLICIT_DEPENDS ${shader_source}
    )
    list(APPEND shader_outputs ${shader_output})
endforeach()

add_custom_target(shaders_spirv DEPENDS ${shader_outputs})

# NOTE: Only include graphics shaders in the embed resources files.
# file(GLOB embed_spirv "${SPIRV_DIR}/graphics_*.spv")
# message(${embed_spirv})
set(path_shadersc "${CMAKE_BINARY_DIR}/_shaders.c")
add_custom_command(
    OUTPUT ${path_shadersc}
    COMMAND ${CMAKE_COMMAND}
    -D DIR="${SPIRV_DIR}/*"
    -D PREFIX="shader"
    -D OUTPUT=${path_shadersc}
    -P "${CMAKE_SOURCE_DIR}/cmake/embed_resources.cmake"
    DEPENDS shaders_spirv ${shader_sources}
    IMPLICIT_DEPENDS shaders_spirv ${shader_sources}
)
add_custom_target(shaders DEPENDS ${path_shadersc})

# -------------------------------------------------------------------------------------------------
# Texture resources
# -------------------------------------------------------------------------------------------------
set(path_tex "${CMAKE_BINARY_DIR}/_textures.c")
set(tex_files
    "${CMAKE_SOURCE_DIR}/data/textures/cmap_atlas.img"
)
add_custom_command(
    OUTPUT ${path_tex}
    COMMAND ${CMAKE_COMMAND}
    -D DIR="${tex_files}"
    -D PREFIX="texture"
    -D OUTPUT=${path_tex}
    -P "${CMAKE_SOURCE_DIR}/cmake/embed_resources.cmake"
)
add_custom_target(textures DEPENDS ${path_tex})

# -------------------------------------------------------------------------------------------------
# Font resources
# -------------------------------------------------------------------------------------------------
set(path_fonts "${CMAKE_BINARY_DIR}/_fonts.c")
set(font_files

    "${CMAKE_SOURCE_DIR}/data/fonts/Roboto-Medium.ttf"
    "${CMAKE_SOURCE_DIR}/data/fonts/Roboto_Medium_atlas.bin" # serialized font atlas

    "${CMAKE_SOURCE_DIR}/data/fonts/Roboto-Regular.ttf"
    "${CMAKE_SOURCE_DIR}/data/fonts/Roboto-Bold.ttf"

    # "${CMAKE_SOURCE_DIR}/data/fonts/NotoSansMono-Light.ttf"
    # "${CMAKE_SOURCE_DIR}/data/fonts/NotoEmoji-Regular.ttf"
    "${CMAKE_SOURCE_DIR}/data/fonts/fontawesome-webfont.ttf")
add_custom_command(
    OUTPUT ${path_fonts}
    COMMAND ${CMAKE_COMMAND}
    -D DIR="${font_files}"
    -D PREFIX="font"
    -D OUTPUT=${path_fonts}
    -P "${CMAKE_SOURCE_DIR}/cmake/embed_resources.cmake"
)
add_custom_target(fonts DEPENDS ${path_fonts})

# -------------------------------------------------------------------------------------------------
# Test data resources
# -------------------------------------------------------------------------------------------------
set(path_testdata "${CMAKE_BINARY_DIR}/_testdata.c")
set(testdata_files
    "${CMAKE_SOURCE_DIR}/data/textures/crate.rgba"
)
add_custom_command(
    OUTPUT ${path_testdata}
    COMMAND ${CMAKE_COMMAND}
    -D DIR="${testdata_files}"
    -D PREFIX="testdata"
    -D OUTPUT=${path_testdata}
    -P "${CMAKE_SOURCE_DIR}/cmake/embed_resources.cmake"
)

# -------------------------------------------------------------------------------------------------
# External sources
# -------------------------------------------------------------------------------------------------

# These files will be compiled with no warnings.
add_library(external_sources STATIC
    "external/fpng.cpp"
    "external/memorymeasure.c"
    "external/vk_mem_alloc.cpp"

    "external/b64/buffer.c"
    "external/b64/encode.c"
    "external/b64/decode.c"

    "external/imgui/imgui.cpp"
    "external/imgui/imgui_demo.cpp"
    "external/imgui/imgui_draw.cpp"
    "external/imgui/imgui_tables.cpp"
    "external/imgui/imgui_widgets.cpp"
    "external/imgui/backends/imgui_impl_glfw.cpp"
    "external/imgui/backends/imgui_impl_vulkan.cpp"
)
target_include_directories(external_sources PRIVATE ${INCL_DIRS})
target_compile_options(external_sources PRIVATE -w)
set_target_properties(external_sources PROPERTIES C_VISIBILITY_PRESET hidden)

# -------------------------------------------------------------------------------------------------
# Datoviz core
# -------------------------------------------------------------------------------------------------
set(core_files

    "src/_atomic.cpp"
    "src/_error.c"
    "src/_list.c"
    "src/_map.cpp"
    "src/_math.c"
    "src/_mutex.c"
    "src/_prng.cpp"
    "src/_thread.c"
    "src/_version.c"
    "src/alloc.c"
    "src/backend.c"
    "src/board.c"
    "src/canvas.c"
    "src/client_input.c"
    "src/client.c"
    "src/context.c"
    "src/datalloc.c"
    "src/external.c"
    "src/fifo.c"
    "src/fileio.cpp"
    "src/fps.cpp"
    "src/gui.cpp"
    "src/host.c"
    "src/input.c"
    "src/keyboard.c"
    "src/log.c"
    "src/loop.c"
    "src/mouse.c"
    "src/pipe.c"
    "src/pipelib.c"
    "src/presenter.c"
    "src/recorder.c"
    "src/renderer.cpp"
    "src/resources.c"
    "src/server.c"
    "src/shader.c"
    "src/surface.c"
    "src/timer.c"
    "src/transfers.c"
    "src/vklite.c"
    "src/window.c"
    "src/workspace.c"
    "src/wrap.c"

    # Qt: the file is protected by #if HAS_QT
    "src/qtapp.cpp"
)

add_library(datoviz_core OBJECT ${core_files})

target_include_directories(datoviz_core PRIVATE ${INCL_DIRS})
target_compile_definitions(datoviz_core PRIVATE ${COMPILE_DEFINITIONS})
target_link_libraries(datoviz_core ${CORE_LIBS})
set_target_properties(datoviz_core PROPERTIES C_VISIBILITY_PRESET hidden)

# -------------------------------------------------------------------------------------------------
# Datoviz requests
# -------------------------------------------------------------------------------------------------
add_library(datoviz_requests OBJECT
    "src/request.c"
)

target_include_directories(datoviz_requests PRIVATE ${INCL_DIRS})
target_compile_definitions(datoviz_requests PRIVATE ${COMPILE_DEFINITIONS})
set_target_properties(datoviz_requests PROPERTIES C_VISIBILITY_PRESET hidden)

# -------------------------------------------------------------------------------------------------
# Datoviz app
# -------------------------------------------------------------------------------------------------
add_library(datoviz_app OBJECT
    "src/app.c"
)

target_include_directories(datoviz_app PRIVATE ${INCL_DIRS})
target_compile_definitions(datoviz_app PRIVATE ${COMPILE_DEFINITIONS})
set_target_properties(datoviz_app PROPERTIES C_VISIBILITY_PRESET hidden)

# -------------------------------------------------------------------------------------------------
# Datoviz scene
# -------------------------------------------------------------------------------------------------
add_library(datoviz_scene OBJECT
    "src/scene/animation.c"
    "src/scene/arcball.c"
    "src/scene/array.c"
    "src/scene/atlas.cpp"
    "src/scene/axis.c"
    "src/scene/axes.c"
    "src/scene/baker.c"
    "src/scene/box.c"
    "src/scene/camera.c"
    "src/scene/colorbar.c"
    "src/scene/colormaps.c"
    "src/scene/demo.c"
    "src/scene/dual.c"
    "src/scene/fly.c"
    "src/scene/font.c"
    "src/scene/geometry.cpp"
    "src/scene/graphics.c"
    "src/scene/grid.c"
    "src/scene/meshobj.cpp"
    "src/scene/mvp.c"
    "src/scene/ortho.c"
    "src/scene/panzoom.c"
    "src/scene/params.c"
    "src/scene/ref.c"
    "src/scene/scene.c"
    "src/scene/sdf.cpp"
    "src/scene/shape.c"
    "src/scene/texture.c"
    "src/scene/ticks.c"
    "src/scene/transform.c"
    "src/scene/viewport.c"
    "src/scene/viewset.c"
    "src/scene/visual.c"
    "src/scene/visuals/basic.c"
    "src/scene/visuals/glyph.c"
    "src/scene/visuals/image.c"
    "src/scene/visuals/wiggle.c"
    "src/scene/visuals/slice.c"
    "src/scene/visuals/marker.c"
    "src/scene/visuals/mesh.c"
    "src/scene/visuals/monoglyph.c"
    "src/scene/visuals/path.c"
    "src/scene/visuals/pixel.c"
    "src/scene/visuals/point.c"
    "src/scene/visuals/segment.c"
    "src/scene/visuals/slice.c"
    "src/scene/visuals/sphere.c"
    "src/scene/visuals/volume.c"
)

target_include_directories(datoviz_scene PRIVATE ${INCL_DIRS})
target_compile_definitions(datoviz_scene PRIVATE ${COMPILE_DEFINITIONS})
target_link_libraries(datoviz_scene ${SCENE_LIBS})
set_target_properties(datoviz_scene PROPERTIES C_VISIBILITY_PRESET hidden)

# -------------------------------------------------------------------------------------------------
# Datoviz resources
# -------------------------------------------------------------------------------------------------
add_library(datoviz_resources OBJECT
    ${path_shadersc}
    ${path_tex}
    ${path_fonts}
)

target_include_directories(datoviz_resources PRIVATE ${INCL_DIRS})
target_compile_definitions(datoviz_resources PRIVATE ${COMPILE_DEFINITIONS})
set_target_properties(datoviz_resources PROPERTIES C_VISIBILITY_PRESET hidden)

# -------------------------------------------------------------------------------------------------
# Datoviz shared library
# -------------------------------------------------------------------------------------------------
add_library(libdatoviz SHARED
    $<TARGET_OBJECTS:datoviz_core>
    $<TARGET_OBJECTS:datoviz_requests>
    $<TARGET_OBJECTS:datoviz_app>
    $<TARGET_OBJECTS:datoviz_scene>
    $<TARGET_OBJECTS:datoviz_resources>

    ${external_sources}
)

add_dependencies(libdatoviz shaders fonts textures)

set_target_properties(libdatoviz PROPERTIES
    VERSION ${DVZ_VERSION}
    SOVERSION 2
    PUBLIC_HEADER include/datoviz.h
    OUTPUT_NAME "datoviz"
)

target_compile_definitions(libdatoviz PUBLIC ${COMPILE_DEFINITIONS})
target_include_directories(libdatoviz PUBLIC ${INCL_DIRS})

# Select list of exported symbols.
# set_target_properties(libdatoviz PROPERTIES
# LINK_FLAGS "-Wl,--version-script=${CMAKE_SOURCE_DIR}/export.map"
# )
if(${OS_LINUX})
    # In DEBUG mode, also add the build directory in the RPATH
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_INSTALL_RPATH "$ORIGIN;${CMAKE_BINARY_DIR}")
    else()
        set(CMAKE_INSTALL_RPATH "$ORIGIN")
    endif()

    target_link_options(libdatoviz PRIVATE
        -Wl,--no-export-dynamic,--retain-symbols-file=${CMAKE_SOURCE_DIR}/symbols.map,--version-script=${CMAKE_SOURCE_DIR}/export.map)
elseif(${OS_MACOS})
    set(CMAKE_INSTALL_RPATH "@loader_path")
endif()

target_link_libraries(libdatoviz
    external_sources
    datoviz_core
    datoviz_requests
    datoviz_app
    datoviz_scene
    datoviz_resources

    # ${LINK_LIBS}
)

set_target_properties(libdatoviz PROPERTIES C_VISIBILITY_PRESET hidden)

# if(${OS_WINDOWS})
# set_target_properties(libdatoviz PROPERTIES OUTPUT_NAME "libdatoviz")
# endif()

# -------------------------------------------------------------------------------------------------
# CLI tool
# -------------------------------------------------------------------------------------------------
if(DATOVIZ_WITH_CLI)
    set(cli_src
        "cli/main.c"
        "tests/testing.cpp"
        "tests/test.c"

        "tests/test_alloc.c"
        "tests/test_app.c"
        "tests/test_board.c"
        "tests/test_canvas.c"
        "tests/test_client_input.c"
        "tests/test_client.c"
        "tests/test_datalloc.c"
        "tests/test_external.c"
        "tests/test_fifo.c"
        "tests/test_fileio.c"
        "tests/test_gui.c"
        "tests/test_host.c"
        "tests/test_input.c"
        "tests/test_keyboard.c"
        "tests/test_list.c"
        "tests/test_loop.c"
        "tests/test_map.c"
        "tests/test_mouse.c"
        "tests/test_obj.c"
        "tests/test_pipe.c"
        "tests/test_pipelib.c"
        "tests/test_presenter.c"
        "tests/test_prng.c"
        "tests/test_renderer.c"
        "tests/test_request.c"
        "tests/test_resources.c"
        "tests/test_server.c"
        "tests/test_thread.c"
        "tests/test_timer.c"
        "tests/test_transfers.c"
        "tests/test_vklite.c"
        "tests/test_window.c"
        "tests/test_workspace.c"

        "tests/scene/test_animation.c"
        "tests/scene/test_arcball.c"
        "tests/scene/test_array.c"
        "tests/scene/test_atlas.c"
        "tests/scene/test_axis.c"
        "tests/scene/test_axes.c"
        "tests/scene/test_baker.c"
        "tests/scene/test_box.c"
        "tests/scene/test_camera.c"
        "tests/scene/test_colormaps.c"
        "tests/scene/test_dual.c"
        "tests/scene/test_font.c"
        "tests/scene/test_graphics.c"
        "tests/scene/test_mvp.c"
        "tests/scene/test_ortho.c"
        "tests/scene/test_panzoom.c"
        "tests/scene/test_params.c"
        "tests/scene/test_ref.c"
        "tests/scene/test_scene.c"
        "tests/scene/test_sdf.c"
        "tests/scene/test_shape.c"
        "tests/scene/test_ticks.c"
        "tests/scene/test_viewset.c"
        "tests/scene/test_visual.c"
        "tests/scene/visuals/test_basic.c"
        "tests/scene/visuals/test_glyph.c"
        "tests/scene/visuals/test_image.c"
        "tests/scene/visuals/test_wiggle.c"
        "tests/scene/visuals/test_slice.c"
        "tests/scene/visuals/test_marker.c"
        "tests/scene/visuals/test_mesh.c"
        "tests/scene/visuals/test_monoglyph.c"
        "tests/scene/visuals/test_path.c"
        "tests/scene/visuals/test_pixel.c"
        "tests/scene/visuals/test_point.c"
        "tests/scene/visuals/test_segment.c"
        "tests/scene/visuals/test_slice.c"
        "tests/scene/visuals/test_sphere.c"
        "tests/scene/visuals/test_volume.c"
    )

    if(${OS_WINDOWS})
        add_executable(cli
            ${cli_src}
            ${path_testdata}
        )
    else()
        add_executable(cli
            ${cli_src}
            ${path_testdata}

            $<TARGET_OBJECTS:datoviz_core>
            $<TARGET_OBJECTS:datoviz_requests>
            $<TARGET_OBJECTS:datoviz_app>
            $<TARGET_OBJECTS:datoviz_scene>
            $<TARGET_OBJECTS:datoviz_resources>
        )
    endif()

    target_include_directories(cli PUBLIC ${INCL_DIRS} ${CMAKE_SOURCE_DIR}/tests)
    target_compile_definitions(cli PUBLIC ${COMPILE_DEFINITIONS})
    target_link_libraries(cli libdatoviz)

    set_target_properties(cli
        PROPERTIES OUTPUT_NAME "datoviz"

        # BUILD_RPATH "${CMAKE_BINARY_DIR}"
        # INSTALL_RPATH "${CMAKE_BINARY_DIR}"
    )
endif()

# -------------------------------------------------------------------------------------------------
# Struct sizes check tool
# -------------------------------------------------------------------------------------------------
add_executable(struct_sizes tools/struct_sizes.c)
target_include_directories(struct_sizes PUBLIC "${CMAKE_SOURCE_DIR}/include")
set_target_properties(struct_sizes PROPERTIES OUTPUT_NAME "struct_sizes")
target_compile_definitions(struct_sizes PUBLIC ${COMPILE_DEFINITIONS})
add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/struct_sizes.json
    COMMAND struct_sizes > ${CMAKE_BINARY_DIR}/struct_sizes.json
    DEPENDS struct_sizes
    COMMENT "Generating struct_sizes.json"
)

# Add a custom target to execute the command
add_custom_target(generate_struct_sizes_json
    ALL
    DEPENDS ${CMAKE_BINARY_DIR}/struct_sizes.json
)
