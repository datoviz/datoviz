# -------------------------------------------------------------------------------------------------
# CMake build script
# -------------------------------------------------------------------------------------------------

set(CMAKE_VERBOSE_MAKEFILE ON)
cmake_minimum_required(VERSION 3.10)
enable_testing()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include("cmake/colormsg.cmake")
include(FetchContent)


# -------------------------------------------------------------------------------------------------
# Datoviz project
# -------------------------------------------------------------------------------------------------

set(DATOVIZ_VERSION 0.2.0)
project(datoviz VERSION ${DATOVIZ_VERSION} DESCRIPTION "datoviz")


# -------------------------------------------------------------------------------------------------
# Datoviz options
# -------------------------------------------------------------------------------------------------

option(DATOVIZ_WITH_GLFW "Build Datoviz with glfw" ON)
option(DATOVIZ_WITH_CLI "Build Datoviz command-line interface with tests and demos" ON)
option(DATOVIZ_WITH_PNG "Build Datoviz with PNG support" ON)
option(DATOVIZ_WITH_VULKAN_SDK "Build Datoviz with Vulkan SDK" ON)
option(DATOVIZ_WITH_GLSLANG "Build Datoviz with glslang support" OFF)
# option(DATOVIZ_WITH_SWIFTSHADER "Build Datoviz with Swiftshader support" OFF)


# -------------------------------------------------------------------------------------------------
# Build type
# -------------------------------------------------------------------------------------------------

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD)
set(CMAKE_C_STANDARD_REQUIRED True)


# -------------------------------------------------------------------------------------------------
# Detect the OS
# -------------------------------------------------------------------------------------------------

set(OS_LINUX 0)
set(OS_MACOS 0)
set(OS_WIN32 0)

if(UNIX AND NOT APPLE)
    set(OS_LINUX 1)
elseif(APPLE)
    set(OS_MACOS 1)
elseif(WIN32)
    set(OS_WIN32 1)
endif()


# -------------------------------------------------------------------------------------------------
# Detect the compiler
# -------------------------------------------------------------------------------------------------

set(CC_GCC 0)
set(CC_CLANG 0)
set(CC_MSVC 0)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CC_GCC 1)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CC_CLANG 1)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
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
    ${CMAKE_SOURCE_DIR}/include/datoviz
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/cimgui
    ${CMAKE_SOURCE_DIR}/external/cimgui/imgui
)

# ---- OS-specific directories --------------------------------------------------------------------
if(${OS_MACOS})
    set(INCL_DIRS ${INCL_DIRS} /usr/local/include)
elseif(${OS_WIN32})
    set(INCL_DIRS ${INCL_DIRS}) #$ENV{VULKAN_SDK}/include $ENV{CGLM_INCLUDE})
    find_path(MINGW_DIR NAMES "gcc.exe")
endif()


# -------------------------------------------------------------------------------------------------
# Linking libraries
# -------------------------------------------------------------------------------------------------

set(LINK_LIBS cglm)
# Link with system libraries.
if(NOT ${WIN32})
    set(LINK_LIBS m dl pthread ${LINK_LIBS})
endif()

# ---- OS-specific linking libraries --------------------------------------------------------------
if(${OS_MACOS})
    link_directories(/usr/local/lib)
elseif(${OS_WIN32})
    # link_directories($ENV{VULKAN_SDK}\\Lib $ENV{VULKAN_SDK}\\Bin) # $ENV{CGLM_LIB} ${MINGW_DIR})
    set(LINK_LIBS ${LINK_LIBS})
endif()


# -------------------------------------------------------------------------------------------------
# Build dependencies
# -------------------------------------------------------------------------------------------------

# ---- cglm ---------------------------------------------------------------------------------------
FetchContent_Declare(
    cglm
    GIT_REPOSITORY  https://github.com/recp/cglm/
    GIT_TAG         v0.8.3
)
FetchContent_MakeAvailable(cglm)

# ---- libpng -------------------------------------------------------------------------------------
set(HAS_PNG 0)
if(DATOVIZ_WITH_PNG)

    if (NOT(OS_WIN32))
        find_package(PNG)
    endif()
    if (PNG_FOUND)
        set(INCL_DIRS ${INCL_DIRS} ${PNG_INCLUDE_DIRS})
        set(LINK_LIBS ${LINK_LIBS} ${PNG_LIBRARIES})
        set(HAS_PNG 1)
    else()
        set(PNG_ARM_NEON off CACHE INTERNAL "")
        set(PNG_ARM_NEON_OPT off CACHE INTERNAL "")
        set(PNG_BUILD_ZLIB ON CACHE BOOL "" FORCE)
        set(PNG_TESTS OFF)
        set(PNG_EXECUTABLES OFF)
        set(PNG_SHARED ON)
        set(SKIP_INSTALL_ALL ON)
        #set(ZLIB_LIBRARY ZLIB::ZLIB)
        #set(AWK "AWK-NOTFOUND")

        # Windows: need to download zlib
        if(OS_WIN32)
            FetchContent_Declare(
                zlib
                GIT_REPOSITORY  https://github.com/datoviz/zlib
                GIT_TAG         master
            )
            FetchContent_MakeAvailable(zlib)
            set(INCL_DIRS ${INCL_DIRS} ${zlib_SOURCE_DIR})
            set(LINK_LIBS ${LINK_LIBS} zlib)

            set(ZLIB_INCLUDE_DIRS ${zlib_SOURCE_DIR})
            set(ZLIB_ROOT ${zlib_SOURCE_DIR})
            set(ZLIB_LIBRARY zlib)
            set(ZLIB_LIBRARIES zlib)
        endif()

        FetchContent_Declare(
            libpng
            GIT_REPOSITORY  https://github.com/datoviz/libpng
            GIT_TAG         libpng16
        )
        FetchContent_MakeAvailable(libpng)

        set(INCL_DIRS ${INCL_DIRS} ${libpng_SOURCE_DIR})
        set(LINK_LIBS ${LINK_LIBS} "${libpng_BINARY_DIR}/libpng16d.lib")
        set(HAS_PNG 1)
    endif()
endif()

# ---- glfw ---------------------------------------------------------------------------------------
set(HAS_GLFW 0)
if(DATOVIZ_WITH_GLFW)
    set(HAS_GLFW 1)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        glfw
        GIT_REPOSITORY  https://github.com/glfw/glfw/
        GIT_TAG         3.3.4
    )
    FetchContent_MakeAvailable(glfw)

    set(INCL_DIRS ${INCL_DIRS} "${glfw_SOURCE_DIR}/include")
    set(LINK_LIBS ${LINK_LIBS} glfw)
endif()

# ---- Vulkan (SDK or bundled) --------------------------------------------------------------------
set(HAS_GLSLANG 0)
if (DATOVIZ_WITH_VULKAN_SDK)
    find_package(Vulkan)
endif()
if (Vulkan_FOUND)
    message(STATUS "Found Vulkan")

    find_program(GLSLC glslc)
    if (NOT GLSLC)
        message(FATAL_ERROR "-- glslc executable NOT found")
    endif()

    # glslang
    set(glslangValidator "")
    if (DATOVIZ_WITH_GLSLANG)
        FetchContent_Declare(
            glslang
            GIT_REPOSITORY https://github.com/KhronosGroup/glslang
            GIT_TAG        11.7.1
        )
        FetchContent_MakeAvailable(glslang)
        set(HAS_GLSLANG 1)
    endif()
else()

    # Vulkan headers
    FetchContent_Declare(
        vulkan-headers
        GIT_REPOSITORY  https://github.com/KhronosGroup/Vulkan-Headers.git
        GIT_TAG         v1.2.195 # main
    )
    FetchContent_MakeAvailable(vulkan-headers)

    # Vulkan loader
    FetchContent_Declare(
        vulkan-loader
        GIT_REPOSITORY  https://github.com/KhronosGroup/Vulkan-Loader.git
        GIT_TAG         v1.2.195 # master
    )
    FetchContent_MakeAvailable(vulkan-loader)

    # SPIRV-Headers
    FetchContent_Declare(
        SPIRV-Headers
        GIT_REPOSITORY  https://github.com/KhronosGroup/SPIRV-Headers.git
        GIT_TAG         master # 1.5.4.raytracing.fixed
    )
    FetchContent_MakeAvailable(SPIRV-Headers)

    # SPIRV-Tools
    FetchContent_Declare(
        SPIRV-Tools
        GIT_REPOSITORY  https://github.com/KhronosGroup/SPIRV-Tools.git
        GIT_TAG         master # v2021.3
        # # TEMPORARY FIX https://github.com/KhronosGroup/SPIRV-Tools/issues/4565
        # GIT_REPOSITORY  https://github.com/jaebaek/SPIRV-Tools.git
        # GIT_TAG         fix_ci_failure
    )
    FetchContent_MakeAvailable(SPIRV-Tools)

    # glslang
    FetchContent_Declare(
        glslang
        GIT_REPOSITORY https://github.com/KhronosGroup/glslang
        GIT_TAG        master # 11.6.0
    )
    FetchContent_MakeAvailable(glslang)

    # glslc
    set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
    set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
    set(SHADERC_SKIP_INSTALL ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
        glslc
        GIT_REPOSITORY https://github.com/google/shaderc
        GIT_TAG        main # v2021.2
        )
    FetchContent_MakeAvailable(glslc)

    set(HAS_GLSLANG 1)
    set(GLSLC glslc_exe)

endif()

# Link with Vulkan.
if(Vulkan_FOUND)
    set(LINK_LIBS ${LINK_LIBS} Vulkan::Vulkan)
else()
    set(LINK_LIBS ${LINK_LIBS} vulkan)
endif()

# Link with glslang.
if (HAS_GLSLANG)
    set(INCL_DIRS ${INCL_DIRS} ${glslang_SOURCE_DIR})
    set(LINK_LIBS ${LINK_LIBS} glslang SPIRV glslang-default-resource-limits)
endif()

# -------------------------------------------------------------------------------------------------
# Compile definitions
# -------------------------------------------------------------------------------------------------

set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS}
    LOG_USE_COLOR
    ENABLE_VALIDATION_LAYERS=1

    # Directories.
    ROOT_DIR=\"${CMAKE_SOURCE_DIR}\"
    DATA_DIR=\"${DATA_DIR}\"
    SPIRV_DIR=\"${SPIRV_DIR}\"
    ARTIFACTS_DIR=\"${ARTIFACTS_DIR}\"

    # OS.
    OS_LINUX=${OS_LINUX}
    OS_MACOS=${OS_MACOS}
    OS_WIN32=${OS_WIN32}

    # Compiler.
    CC_MSVC=${CC_MSVC}
    CC_GCC=${CC_GCC}
    CC_CLANG=${CC_CLANG}

    # Dependencies.
    HAS_GLFW=${HAS_GLFW}
    HAS_GLSLANG=${HAS_GLSLANG}
    HAS_PNG=${HAS_PNG}
)


# -------------------------------------------------------------------------------------------------
# Compiler options
# -------------------------------------------------------------------------------------------------

# ---- common compiler options --------------------------------------------------------------------
# NOTE: need to remove -pg (gprof profiling) in RELEASE mode?
set(COMMON_FLAGS "-g -m64 -pedantic \
    -Wall \
    -Wextra \
    -Werror=vla \
    -Wcast-align \
    -Wcast-qual \
    -Wredundant-decls \
    -Wswitch-default \
    -Wdisabled-optimization \
    -Wmissing-declarations \
    -Wmissing-include-dirs \
    -Wshadow \
    -Wsign-conversion \
    -Wstrict-overflow=5 \
    -Wundef \
    -Wformat=2 -Winit-self \
    -Wno-unused-function \
    -Wno-unused-parameter \
    -Wno-format-nonliteral \
    ")
    # -Wno-missing-field-initializers
    # -Wno-unused-result \
    # -Wno-variadic-macros")

# ---- compiler-specific options ------------------------------------------------------------------
if(CC_GCC)
    # gcc
    set(COMMON_FLAGS "${COMMON_FLAGS} \
        -Wlogical-op -fdiagnostics-color=always")
    if (NOT OS_WIN32)
        set(COMMON_FLAGS "${COMMON_FLAGS} -pg")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} \
        -std=gnu11 -Wmissing-prototypes -Wimplicit-function-declaration")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -std=c++17")

elseif(CC_CLANG)
    # clang
    set(COMMON_FLAGS "${COMMON_FLAGS} \
        -Wno-unused-parameter \
        -fcolor-diagnostics \
        -Wno-missing-braces \
        -Wmissing-prototypes \
        ")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -std=c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -std=c++17")
    set(CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS} ${COMMON_FLAGS}
        -Wno-partial-availability -Wno-unguarded-availability-new")

elseif(CC_MSVC)
    # MSVC

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")

endif()


# -------------------------------------------------------------------------------------------------
# RPATH
# see https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
# -------------------------------------------------------------------------------------------------

# use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif("${isSystemDir}" STREQUAL "-1")


# -------------------------------------------------------------------------------------------------
# Shaders compilation
# -------------------------------------------------------------------------------------------------

find_program(GLSLC glslc)
file(GLOB shader_sources
    "*/glsl/*.vert"
    "*/glsl/*.geom"
    "*/glsl/*.comp"
    "*/glsl/*.frag"
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
            -I "${CMAKE_SOURCE_DIR}/include/datoviz/glsl"
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
# Other binary resources
# -------------------------------------------------------------------------------------------------

# Color texture.
set(path_colortex "${CMAKE_BINARY_DIR}/_colortex.c")
add_custom_command(
    OUTPUT ${path_colortex}
    COMMAND ${CMAKE_COMMAND}
    -D DIR="${CMAKE_SOURCE_DIR}/data/textures/cmap_atlas.img"
    -D PREFIX="texture"
    -D OUTPUT=${path_colortex}
    -P "${CMAKE_SOURCE_DIR}/cmake/embed_resources.cmake"
)

# Fonts.
set(path_fonts "${CMAKE_BINARY_DIR}/_fonts.c")
set(font_files
    "${CMAKE_SOURCE_DIR}/data/textures/font_inconsolata.png"
    "${CMAKE_SOURCE_DIR}/data/fonts/Roboto-Medium.ttf"
    "${CMAKE_SOURCE_DIR}/data/fonts/fontawesome-webfont.ttf")
add_custom_command(
    OUTPUT ${path_fonts}
    COMMAND ${CMAKE_COMMAND}
    -D DIR="${font_files}"
    -D PREFIX="font"
    -D OUTPUT=${path_fonts}
    -P "${CMAKE_SOURCE_DIR}/cmake/embed_resources.cmake"
)


# -------------------------------------------------------------------------------------------------
# Source files
# -------------------------------------------------------------------------------------------------

# These files will be compiled with no warnings.
set(external_sources
    "external/tinycthread.c"
    "external/vk_mem_alloc.cpp"

    "external/cimgui/imgui/imgui.cpp"
    "external/cimgui/imgui/imgui_demo.cpp"
    "external/cimgui/imgui/imgui_draw.cpp"
    "external/cimgui/imgui/imgui_tables.cpp"
    "external/cimgui/imgui/imgui_widgets.cpp"
    "external/cimgui/imgui/backends/imgui_impl_glfw.cpp"
    "external/cimgui/imgui/backends/imgui_impl_vulkan.cpp"
    "external/cimgui/cimgui.cpp"
)

set(sources
    "src/_thread.c"
    "src/_mutex.c"
    "src/_atomic.cpp"
    "src/_prng.cpp"
    "src/log.c"
    "src/fileio.c"
    "src/fifo.c"
    "src/array.c"
    "src/host.c"
    "src/window.c"
    "src/vklite.c"
    "src/spirv.c"
    "src/alloc.cpp"
    "src/resources.c"
    "src/datalloc.c"
    "src/transfers.c"
    "src/context.c"
    "src/board.c"
    "src/pipe.c"
    "src/pipelib.c"
    "src/graphics.c"
    "src/graphics_data.c"
    "src/gui.c"
    "src/workspace.c"
    "src/request.c"
    "src/map.cpp"
    "src/renderer.cpp"
    "src/input.c"

    ${external_sources}
)



# -------------------------------------------------------------------------------------------------
# Datoviz shared library
# -------------------------------------------------------------------------------------------------

add_library(libdatoviz SHARED ${sources} ${path_shadersc} ${path_colortex} ${path_fonts})
set_source_files_properties(${external_sources} PROPERTIES COMPILE_FLAGS -w)
set_target_properties(libdatoviz PROPERTIES
    VERSION ${DATOVIZ_VERSION}
    SOVERSION 1
    # PUBLIC_HEADER include/datoviz/datoviz.h
    OUTPUT_NAME "datoviz"
    )
if(${OS_WIN32})
    set_target_properties(
        libdatoviz PROPERTIES
        OUTPUT_NAME "libdatoviz"
    )
endif()
add_dependencies(libdatoviz shaders)
target_compile_definitions(libdatoviz PUBLIC ${COMPILE_DEFINITIONS})
target_include_directories(libdatoviz PUBLIC ${INCL_DIRS})
target_link_libraries(libdatoviz ${LINK_LIBS})


# -------------------------------------------------------------------------------------------------
# CLI tool
# -------------------------------------------------------------------------------------------------

if (DATOVIZ_WITH_CLI)
    set(cli
        "cli/main.c"
        "tests/test_thread.c"
        "tests/test_obj.c"
        "tests/test_fileio.c"
        "tests/test_fifo.c"
        "tests/test_array.c"
        "tests/test_window.c"
        "tests/test_vklite.c"
        "tests/test_alloc.c"
        "tests/test_resources.c"
        "tests/test_datalloc.c"
        "tests/test_transfers.c"
        "tests/test_board.c"
        "tests/test_pipe.c"
        "tests/test_pipelib.c"
        "tests/test_graphics.c"
        "tests/test_workspace.c"
        "tests/test_map.c"
        "tests/test_request.c"
        "tests/test_renderer.c"
        "tests/test.c"
    )

    add_executable(datovizcli ${cli})
    set_target_properties(datovizcli PROPERTIES OUTPUT_NAME "datoviz")
    target_compile_definitions(datovizcli PUBLIC ${COMPILE_DEFINITIONS})
    target_include_directories(datovizcli PUBLIC ${INCL_DIRS} ${CMAKE_SOURCE_DIR}/tests)
    target_link_libraries(datovizcli libdatoviz)
endif()
