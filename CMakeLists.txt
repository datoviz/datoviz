# -------------------------------------------------------------------------------------------------
# Meta CMake file
# -------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.21)
project(datoviz VERSION 0.4 LANGUAGES C CXX)

include(GNUInstallDirs)
include(cmake/DvzShaders.cmake)

# Threading setup (portable: Linux, macOS, Windows-MSVC, Windows-MinGW)
if(WIN32)
    if(MINGW)
        # MinGW-w64 provides pthreads automatically (winpthread)
        set(THREADS_PREFER_PTHREAD_FLAG ON)
        set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
        find_package(Threads REQUIRED)
        set(DVZ_THREADING_TARGET Threads::Threads)
    else()
        # True MSVC / Visual Studio environment → use PThreads4W
        find_package(PThreads4W CONFIG REQUIRED)
        set(DVZ_THREADING_TARGET PThreads4W::PThreads4W)
    endif()
else()
    # Unix-like platforms: regular pthreads
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
    find_package(Threads REQUIRED)
    set(DVZ_THREADING_TARGET Threads::Threads)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Allocator & sanitizer options
option(DVZ_ENABLE_ASAN_IN_DEBUG "Enable sanitizer instrumentation in Debug builds" OFF)
option(DVZ_ENABLE_COVERAGE "Enable code coverage instrumentation for supported compilers" OFF)
option(DVZ_ENABLE_GPROF "Enable gprof instrumentation (-pg) on supported compilers" OFF)
set(DVZ_SANITIZER "" CACHE STRING "Optional sanitizer override for Debug builds (asan, msan, tsan, or a comma-separated list)")
set_property(CACHE DVZ_SANITIZER PROPERTY STRINGS "" "asan" "msan" "tsan" "off")
option(DVZ_USE_MIMALLOC_RELEASE_DEFAULT "Use mimalloc as the default allocator in Release builds" ON)

set(_dvz_enable_cuda_default ON)

if(APPLE)
    set(_dvz_enable_cuda_default OFF)
endif()

option(DVZ_ENABLE_CUDA "Enable CUDA interop hooks and tests" ${_dvz_enable_cuda_default})
option(DVZ_BUILD_CORE "Build the datoviz core layer (common/fileio/geom/math/thread)" ON)
option(DVZ_BUILD_CONTROLLER "Build scene-independent camera/controller primitives" ON)
option(DVZ_BUILD_VK "Build the Vulkan layer (vk/vklite)" ON)
option(DVZ_BUILD_CANVAS "Build the canvas layer (input/window/stream/video/canvas)" ON)
option(DVZ_BUILD_DRP2 "Build the DRP2 layer" ON)
option(DVZ_BUILD_WEBGPU "Build the WebGPU layer" OFF)
option(DVZ_BUILD_SCENE "Build the scene layer" ON)
option(DVZ_BUILD_APP "Build the app presentation layer" ON)
option(DVZ_BUILD_GUI "Build the Dear ImGui app overlay layer" ON)
option(DVZ_BUILD_TESTING "Build Datoviz test executables" ${PROJECT_IS_TOP_LEVEL})
option(DVZ_BUILD_EXAMPLES "Build Datoviz example executables" ${PROJECT_IS_TOP_LEVEL})
option(DVZ_BUILD_IMPLOT_EXAMPLE "Build the ImPlot integration example" OFF)
option(DVZ_INSTALL "Install Datoviz targets, headers, and package metadata" ${PROJECT_IS_TOP_LEVEL})
option(DVZ_VENDORED_DEPS "Use bundled third-party dependency source trees when available" ON)
set(DVZ_IMPLOT_SOURCE_DIR "" CACHE PATH "Optional local ImPlot v1.0 source checkout")
set(DVZ_MIMALLOC_SOURCE "AUTO" CACHE STRING "mimalloc source mode (AUTO, SYSTEM, VENDORED, or OFF)")
set(DVZ_CGLM_SOURCE "AUTO" CACHE STRING "cglm source mode (AUTO, SYSTEM, VENDORED, or OFF)")
set(DVZ_KVAZAAR_SOURCE "AUTO" CACHE STRING "Kvazaar source mode (AUTO, SYSTEM, VENDORED, or OFF)")
set_property(CACHE DVZ_MIMALLOC_SOURCE PROPERTY STRINGS AUTO SYSTEM VENDORED OFF)
set_property(CACHE DVZ_CGLM_SOURCE PROPERTY STRINGS AUTO SYSTEM VENDORED OFF)
set_property(CACHE DVZ_KVAZAAR_SOURCE PROPERTY STRINGS AUTO SYSTEM VENDORED OFF)
option(DVZ_WITH_FREETYPE "Enable FreeType-backed scene text atlases when available" ON)
option(DVZ_WITH_MSDF_ATLAS "Enable msdf-atlas-gen scene text atlases when available" ON)
option(DVZ_WITH_MSDF_SVG "Enable SVG path import through msdfgen when tinyxml2 is available" ON)
set(DVZ_ENABLE_QT_BRIDGE "AUTO" CACHE STRING "Build the optional Qt bridge provider (AUTO, ON, or OFF)")
set_property(CACHE DVZ_ENABLE_QT_BRIDGE PROPERTY STRINGS AUTO ON OFF)
set(DVZ_ENABLE_SHADERC "AUTO" CACHE STRING "Enable runtime GLSL compilation through lazy-loaded shaderc (AUTO, ON, or OFF)")
set_property(CACHE DVZ_ENABLE_SHADERC PROPERTY STRINGS AUTO ON OFF)
set(DVZ_SHADERC_INCLUDE_DIR "" CACHE PATH "Optional shaderc include directory containing shaderc/shaderc.h")
set(DVZ_SHADERC_LIBRARY "" CACHE FILEPATH "Optional shaderc shared library to lazy-load at runtime")
set(DVZ_SHADERC_RUNTIME_LIBRARY "" CACHE STRING "Optional shaderc runtime library name or path baked into Datoviz")
set(DVZ_MSDF_ATLAS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/external/msdf-atlas-gen" CACHE PATH
    "Optional local msdf-atlas-gen source directory")
set(DVZ_HAS_QT_BRIDGE 0 CACHE INTERNAL "Qt bridge provider availability flag" FORCE)
set(DVZ_HAS_SHADERC 0 CACHE INTERNAL "shaderc compile-time header availability flag" FORCE)
set(DVZ_SHADERC_STATIC 0 CACHE INTERNAL "shaderc is linked statically into Datoviz" FORCE)
set(DVZ_SHADERC_TARGET "" CACHE INTERNAL "CMake target providing statically linked shaderc" FORCE)

if(WIN32 AND NOT DVZ_ENABLE_SHADERC STREQUAL "OFF")
    find_package(unofficial-shaderc CONFIG QUIET)
    if(TARGET unofficial::shaderc::shaderc)
        set(DVZ_HAS_SHADERC 1 CACHE INTERNAL "shaderc compile-time header availability flag" FORCE)
        set(DVZ_SHADERC_STATIC 1 CACHE INTERNAL "shaderc is linked statically into Datoviz" FORCE)
        set(DVZ_SHADERC_TARGET unofficial::shaderc::shaderc CACHE INTERNAL
            "CMake target providing statically linked shaderc" FORCE)
    elseif(DVZ_ENABLE_SHADERC STREQUAL "ON")
        message(FATAL_ERROR
            "DVZ_ENABLE_SHADERC=ON on Windows requires the vcpkg shaderc package exporting "
            "unofficial::shaderc::shaderc. Configure with the vcpkg toolchain and install shaderc.")
    endif()
endif()

if(NOT DVZ_HAS_SHADERC AND NOT DVZ_ENABLE_SHADERC STREQUAL "OFF")
    find_path(_dvz_shaderc_include_dir
        NAMES shaderc/shaderc.h
        HINTS "${DVZ_SHADERC_INCLUDE_DIR}" "$ENV{VULKAN_SDK}/include"
    )
    find_library(_dvz_shaderc_library
        NAMES shaderc_shared shaderc
        HINTS "$ENV{VULKAN_SDK}/lib"
    )

    if(_dvz_shaderc_include_dir)
        set(DVZ_SHADERC_INCLUDE_DIR "${_dvz_shaderc_include_dir}" CACHE PATH
            "Optional shaderc include directory containing shaderc/shaderc.h" FORCE)
        set(DVZ_HAS_SHADERC 1 CACHE INTERNAL "shaderc compile-time header availability flag" FORCE)
        if(_dvz_shaderc_library AND NOT DVZ_SHADERC_LIBRARY)
            set(DVZ_SHADERC_LIBRARY "${_dvz_shaderc_library}" CACHE FILEPATH
                "Optional shaderc shared library to lazy-load at runtime" FORCE)
        endif()
    elseif(DVZ_ENABLE_SHADERC STREQUAL "ON")
        message(FATAL_ERROR "DVZ_ENABLE_SHADERC=ON but shaderc/shaderc.h was not found.")
    endif()
endif()

if(DVZ_HAS_SHADERC)
    if(DVZ_SHADERC_STATIC)
        message(STATUS "shaderc support enabled; runtime GLSL compilation is statically linked")
    else()
        set(_dvz_shaderc_old_default "")
        if(APPLE)
            set(_dvz_shaderc_old_default "@loader_path/libshaderc_shared.1.dylib")
        endif()
        if(DVZ_SHADERC_RUNTIME_LIBRARY
                AND NOT DVZ_SHADERC_RUNTIME_LIBRARY STREQUAL "${_dvz_shaderc_old_default}")
            set(_dvz_shaderc_runtime "${DVZ_SHADERC_RUNTIME_LIBRARY}")
        endif()
        if(NOT _dvz_shaderc_runtime)
            if(WIN32)
                set(_dvz_shaderc_runtime "libshaderc_shared.dll")
            elseif(APPLE)
                set(_dvz_shaderc_runtime "@rpath/libshaderc_shared.1.dylib")
            else()
                set(_dvz_shaderc_runtime "libshaderc_shared.so.1")
            endif()
        endif()
        set(DVZ_SHADERC_RUNTIME_LIBRARY "${_dvz_shaderc_runtime}" CACHE STRING
            "Optional shaderc runtime library name or path baked into Datoviz" FORCE)
        message(STATUS
            "shaderc support enabled; runtime GLSL compilation will lazy-load: "
            "${DVZ_SHADERC_RUNTIME_LIBRARY}")
    endif()
else()
    message(STATUS
        "shaderc support disabled; runtime GLSL compilation will be unavailable. "
        "Install shaderc development files or the Vulkan SDK, then configure with "
        "-DDVZ_ENABLE_SHADERC=ON to require it for release wheels.")
endif()

if(DVZ_ENABLE_GPROF)
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C>:-pg>
        $<$<COMPILE_LANGUAGE:CXX>:-pg>
    )
    add_link_options(-pg)
endif()

# Track CUDA interop and NVENC video support independently so CUDA external-memory users do not
# require an encoder library.
set(DVZ_HAS_CUDA 0)
set(DVZ_HAS_NVENC 0)
set(DVZ_NVENC_LIB "")
set(DVZ_NVENC_LIB_DIR "")
option(DVZ_ENABLE_KVAZAAR "Enable Kvazaar software HEVC backend" ON)
option(DVZ_WITH_GLFW "Enable the GLFW window backend" ON)
option(DVZ_WITH_ZLIB "Enable zlib support for gzip-compressed data files" ON)
set(DVZ_HAS_KVZ 0)
set(DVZ_HAS_GLFW 0)
set(DVZ_HAS_ZLIB 0)
set(DVZ_HAS_CGLM 0)

include(cmake/dependencies/source_modes.cmake)

set(DVZ_GLFW_DIR "${PROJECT_SOURCE_DIR}/external/glfw")

if(DVZ_WITH_GLFW)
    if(DVZ_VENDORED_DEPS)
        if(EXISTS "${DVZ_GLFW_DIR}/CMakeLists.txt")
            set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
            set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
            set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

            add_subdirectory(
                "${DVZ_GLFW_DIR}"
                "${CMAKE_CURRENT_BINARY_DIR}/external/glfw"
                EXCLUDE_FROM_ALL)

            if(TARGET glfw)
                set(DVZ_HAS_GLFW 1)
                if(NOT TARGET glfw::glfw)
                    add_library(glfw::glfw ALIAS glfw)
                endif()
            endif()
        else()
            message(WARNING "GLFW submodule missing — disable DVZ_WITH_GLFW or run 'git submodule update --init external/glfw'")
        endif()
    else()
        find_package(glfw3 QUIET)

        if(TARGET glfw::glfw)
            set(DVZ_HAS_GLFW 1)
        elseif(TARGET glfw)
            set(DVZ_HAS_GLFW 1)
            add_library(glfw::glfw ALIAS glfw)
        endif()
    endif()
endif()

if(DVZ_WITH_GLFW AND NOT DVZ_HAS_GLFW)
    if(DVZ_VENDORED_DEPS)
        message(WARNING "DVZ_WITH_GLFW is ON but GLFW was not built; falling back to headless window backend.")
    else()
        message(WARNING "DVZ_WITH_GLFW is ON but system GLFW was not found; falling back to headless window backend.")
    endif()
endif()

set(DVZ_HAS_GLFW "${DVZ_HAS_GLFW}" CACHE INTERNAL "GLFW availability flag" FORCE)

if(DVZ_WITH_ZLIB)
    find_package(ZLIB QUIET)

    if(ZLIB_FOUND)
        set(DVZ_HAS_ZLIB 1)
    else()
        message(WARNING "DVZ_WITH_ZLIB is ON but zlib was not found; gzip loading is disabled.")
    endif()
endif()

set(DVZ_HAS_ZLIB "${DVZ_HAS_ZLIB}" CACHE INTERNAL "zlib availability flag" FORCE)

set(DVZ_EXTERNAL_DIR "${PROJECT_SOURCE_DIR}/external")
include(cmake/dependencies/cglm.cmake)

set(DVZ_HAS_FREETYPE 0)
set(DVZ_HAS_MSDF_ATLAS 0)
set(DVZ_HAS_MSDF_SVG 0)
set(DVZ_TEXT_ATLAS_LIBS "")

if(DVZ_BUILD_SCENE AND DVZ_WITH_FREETYPE)
    find_package(Freetype QUIET)
    if(Freetype_FOUND)
        set(DVZ_HAS_FREETYPE 1)
        list(APPEND DVZ_TEXT_ATLAS_LIBS Freetype::Freetype)
    else()
        message(WARNING "DVZ_WITH_FREETYPE is ON but FreeType was not found; hinted bitmap text is disabled.")
    endif()
endif()

if(DVZ_BUILD_SCENE AND DVZ_WITH_MSDF_ATLAS)
    set(MSDFGEN_CORE_ONLY OFF CACHE BOOL "" FORCE)
    set(MSDFGEN_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
    set(MSDFGEN_USE_VCPKG OFF CACHE BOOL "" FORCE)
    set(MSDFGEN_USE_SKIA OFF CACHE BOOL "" FORCE)
    if(DVZ_WITH_MSDF_SVG)
        find_package(tinyxml2 QUIET)
        if(TARGET tinyxml2::tinyxml2)
            set(DVZ_HAS_MSDF_SVG 1)
            set(MSDFGEN_DISABLE_SVG OFF CACHE BOOL "" FORCE)
        else()
            set(MSDFGEN_DISABLE_SVG ON CACHE BOOL "" FORCE)
            message(WARNING "DVZ_WITH_MSDF_SVG is ON but tinyxml2 was not found; "
                "SVG path import is disabled.")
        endif()
    else()
        set(MSDFGEN_DISABLE_SVG ON CACHE BOOL "" FORCE)
    endif()
    set(MSDFGEN_DISABLE_PNG ON CACHE BOOL "" FORCE)
    set(MSDF_ATLAS_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
    set(MSDF_ATLAS_USE_VCPKG OFF CACHE BOOL "" FORCE)
    set(MSDF_ATLAS_USE_SKIA OFF CACHE BOOL "" FORCE)
    set(MSDF_ATLAS_NO_ARTERY_FONT ON CACHE BOOL "" FORCE)
    if(MSVC)
        set(MSDF_ATLAS_DYNAMIC_RUNTIME ON CACHE BOOL "" FORCE)
    else()
        set(MSDF_ATLAS_DYNAMIC_RUNTIME OFF CACHE BOOL "" FORCE)
    endif()
    set(MSDF_ATLAS_MSDFGEN_EXTERNAL OFF CACHE BOOL "" FORCE)
    set(MSDF_ATLAS_INSTALL OFF CACHE BOOL "" FORCE)

    if(NOT DVZ_HAS_FREETYPE)
        message(WARNING "DVZ_WITH_MSDF_ATLAS is ON but FreeType was not found; MSDF text is disabled.")
    else()
        if(NOT TARGET msdf-atlas-gen::msdf-atlas-gen AND NOT TARGET msdf-atlas-gen AND
                EXISTS "${DVZ_MSDF_ATLAS_SOURCE_DIR}/CMakeLists.txt")
            add_subdirectory(
                "${DVZ_MSDF_ATLAS_SOURCE_DIR}"
                "${CMAKE_CURRENT_BINARY_DIR}/external/msdf-atlas-gen"
                EXCLUDE_FROM_ALL)
        endif()

        if(NOT TARGET msdf-atlas-gen::msdf-atlas-gen AND NOT TARGET msdf-atlas-gen)
            find_package(msdf-atlas-gen CONFIG QUIET)
        endif()

        if(NOT TARGET msdf-atlas-gen::msdf-atlas-gen AND NOT TARGET msdf-atlas-gen AND
                DVZ_MSDF_ATLAS_SOURCE_DIR AND
                NOT EXISTS "${DVZ_MSDF_ATLAS_SOURCE_DIR}/CMakeLists.txt")
            message(STATUS "msdf-atlas-gen source directory not found: ${DVZ_MSDF_ATLAS_SOURCE_DIR}")
        endif()

        if(NOT TARGET msdf-atlas-gen::msdf-atlas-gen AND NOT TARGET msdf-atlas-gen)
            message(WARNING "DVZ_WITH_MSDF_ATLAS is ON but msdf-atlas-gen was not found locally; "
                "MSDF text is disabled. Initialize external/msdf-atlas-gen or set "
                "DVZ_MSDF_ATLAS_SOURCE_DIR to a local checkout.")
        endif()

        if(TARGET msdf-atlas-gen::msdf-atlas-gen)
            set(DVZ_HAS_MSDF_ATLAS 1)
            list(APPEND DVZ_TEXT_ATLAS_LIBS msdf-atlas-gen::msdf-atlas-gen)
        elseif(TARGET msdf-atlas-gen)
            set(DVZ_HAS_MSDF_ATLAS 1)
            list(APPEND DVZ_TEXT_ATLAS_LIBS msdf-atlas-gen)
        endif()
    endif()
endif()

set(DVZ_HAS_FREETYPE "${DVZ_HAS_FREETYPE}" CACHE INTERNAL "FreeType availability flag" FORCE)
set(DVZ_HAS_MSDF_ATLAS "${DVZ_HAS_MSDF_ATLAS}" CACHE INTERNAL "msdf-atlas-gen availability flag" FORCE)
set(DVZ_HAS_MSDF_SVG "${DVZ_HAS_MSDF_SVG}" CACHE INTERNAL "msdfgen SVG import availability flag" FORCE)

if(DVZ_ENABLE_CUDA)
    find_package(CUDAToolkit QUIET)

    if(NOT CUDAToolkit_FOUND)
        message(WARNING "CUDA toolkit not found — disabling DVZ_ENABLE_CUDA")
        set(DVZ_ENABLE_CUDA OFF CACHE BOOL "Enable CUDA interop hooks and tests" FORCE)
    else()
        set(DVZ_HAS_CUDA 1)

        if(WIN32)
            set(_dvz_nvenc_lib_dir "${PROJECT_SOURCE_DIR}/libs/nvenc/windows")
        elseif(APPLE)
            set(_dvz_nvenc_lib_dir "${PROJECT_SOURCE_DIR}/libs/nvenc/macos")
        else()
            set(_dvz_nvenc_lib_dir "${PROJECT_SOURCE_DIR}/libs/nvenc/linux")
        endif()

        set(_dvz_nvenc_names nvencodeapi nvencodeapi64)

        if(UNIX AND NOT APPLE)
            list(APPEND _dvz_nvenc_names nvidia-encode)
        endif()

        find_library(_dvz_nvenc_lib
            NAMES ${_dvz_nvenc_names}
            HINTS ${_dvz_nvenc_lib_dir}
        )

        if(_dvz_nvenc_lib)
            set(DVZ_HAS_NVENC 1)
            # Set normal variables for this configure pass because the empty defaults above shadow
            # same-named cache entries.
            set(DVZ_NVENC_LIB "${_dvz_nvenc_lib}")
            set(DVZ_NVENC_LIB_DIR "${_dvz_nvenc_lib_dir}")
            set(DVZ_NVENC_LIB "${_dvz_nvenc_lib}" CACHE FILEPATH "NVENC library resolved by CMake" FORCE)
            set(DVZ_NVENC_LIB_DIR "${_dvz_nvenc_lib_dir}" CACHE PATH "Directory searched for NVENC" FORCE)
        else()
            message(STATUS "CUDA toolkit found but NVENC library missing — CUDA interop remains enabled; NVENC video is unavailable")
        endif()
    endif()
endif()

if(NOT DVZ_HAS_NVENC)
    unset(DVZ_NVENC_LIB CACHE)
    unset(DVZ_NVENC_LIB_DIR CACHE)
endif()

set(DVZ_HAS_CUDA "${DVZ_HAS_CUDA}" CACHE INTERNAL "CUDA availability flag" FORCE)
set(DVZ_HAS_NVENC "${DVZ_HAS_NVENC}" CACHE INTERNAL "NVENC availability flag" FORCE)

include(cmake/dependencies/kvazaar.cmake)
include(cmake/dependencies/mimalloc.cmake)

# Optional: default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Enable testing (for ctest)
# enable_testing()

# Global include for public headers
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# Prepare a shared list of targets so subdirectories can register themselves.
set_property(GLOBAL PROPERTY DVZ_ALL_TARGETS "")

# -------------------------------------------------------------------------------------------------
# Shader precompilation — builtin scene visual shaders compiled to SPIR-V at build time.
# -------------------------------------------------------------------------------------------------

set(GLSL_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/scene/shaders/glsl")
if(EXISTS "${GLSL_SOURCE_DIR}")
    file(GLOB GLSL_STAGE_SOURCES
        "${GLSL_SOURCE_DIR}/*.vert"
        "${GLSL_SOURCE_DIR}/*.frag"
        "${GLSL_SOURCE_DIR}/*.comp")
    file(GLOB GLSL_TEXT_SOURCES
        "${GLSL_SOURCE_DIR}/*.vert"
        "${GLSL_SOURCE_DIR}/*.frag"
        "${GLSL_SOURCE_DIR}/*.comp"
        "${GLSL_SOURCE_DIR}/*.glsl")
    file(GLOB GLSL_INCLUDES "${GLSL_SOURCE_DIR}/*.glsl")
else()
    set(GLSL_STAGE_SOURCES "")
    set(GLSL_TEXT_SOURCES "")
    set(GLSL_INCLUDES "")
endif()

set(DVZ_EMBED_RESOURCES_SCRIPT "${PROJECT_SOURCE_DIR}/cmake/embed_resources.cmake")
set(DVZ_EMBED_TEXT_RESOURCES_SCRIPT "${PROJECT_SOURCE_DIR}/cmake/embed_text_resources.cmake")

set(SHADERS_C "${CMAKE_BINARY_DIR}/_shaders.c")
if(GLSLC)
    set(SPIRV_DIR       "${CMAKE_BINARY_DIR}/shaders")
    file(MAKE_DIRECTORY "${SPIRV_DIR}")

    set(SPIRV_OUTPUTS "")
    foreach(glsl_src ${GLSL_STAGE_SOURCES})
        get_filename_component(shader_name "${glsl_src}" NAME)
        get_filename_component(shader_ext "${glsl_src}" EXT)
        set(spv_out "${SPIRV_DIR}/${shader_name}.spv")
        if(shader_ext STREQUAL ".comp")
            set(shader_stage compute)
            set(shader_profile compute)
        elseif(shader_ext STREQUAL ".vert")
            set(shader_stage vertex)
            set(shader_profile graphics)
        else()
            set(shader_stage fragment)
            set(shader_profile graphics)
        endif()
        dvz_compile_glsl(
            SOURCE "${glsl_src}"
            OUTPUT "${spv_out}"
            STAGE "${shader_stage}"
            PROFILE "${shader_profile}"
            INCLUDE_DIRS "${GLSL_SOURCE_DIR}"
            DEPENDS ${GLSL_INCLUDES}
        )
        list(APPEND SPIRV_OUTPUTS "${spv_out}")
    endforeach()

    add_custom_target(datoviz_shaders_spirv DEPENDS ${SPIRV_OUTPUTS})

    # Pass the file list with semicolons escaped so cmake -P receives a proper CMake list.
    string(REPLACE ";" "\\;" _spirv_list_escaped "${SPIRV_OUTPUTS}")
    add_custom_command(
        OUTPUT  "${SHADERS_C}"
        COMMAND "${CMAKE_COMMAND}"
            "-DFILES=${_spirv_list_escaped}"
            "-DPREFIX=shader"
            "-DOUTPUT=${SHADERS_C}"
            -P "${DVZ_EMBED_RESOURCES_SCRIPT}"
        DEPENDS datoviz_shaders_spirv ${SPIRV_OUTPUTS} "${DVZ_EMBED_RESOURCES_SCRIPT}"
        COMMENT "Embedding SPIR-V shaders into _shaders.c"
    )
    add_custom_target(datoviz_shaders DEPENDS "${SHADERS_C}")

    set(DVZ_HAS_PRECOMPILED_SHADERS 1)
    set(DVZ_SHADERS_C "${SHADERS_C}")
else()
    if(DVZ_REQUIRE_PRECOMPILED_SHADERS)
        message(FATAL_ERROR "DVZ_REQUIRE_PRECOMPILED_SHADERS=ON requires glslc.")
    endif()
    message(WARNING "glslc not found — builtin shaders will use runtime GLSL compilation.")
    add_custom_command(
        OUTPUT  "${SHADERS_C}"
        COMMAND "${CMAKE_COMMAND}"
            "-DFILES="
            "-DPREFIX=shader"
            "-DOUTPUT=${SHADERS_C}"
            -P "${DVZ_EMBED_RESOURCES_SCRIPT}"
        DEPENDS "${DVZ_EMBED_RESOURCES_SCRIPT}"
        COMMENT "Generating empty SPIR-V shader resource registry"
    )
    add_custom_target(datoviz_shaders DEPENDS "${SHADERS_C}")
    set(DVZ_HAS_PRECOMPILED_SHADERS 0)
    set(DVZ_SHADERS_C "${SHADERS_C}")
endif()

# -------------------------------------------------------------------------------------------------
# GLSL embedding — runtime fallback sources for built-in scene visual shaders.
# -------------------------------------------------------------------------------------------------

if(GLSL_TEXT_SOURCES)
    set(GLSL_SHADERS_C "${CMAKE_BINARY_DIR}/_glsl_shaders.c")
    string(REPLACE ";" "\\;" _glsl_list_escaped "${GLSL_TEXT_SOURCES}")
    add_custom_command(
        OUTPUT  "${GLSL_SHADERS_C}"
        COMMAND "${CMAKE_COMMAND}"
            "-DFILES=${_glsl_list_escaped}"
            "-DPREFIX=glsl"
            "-DOUTPUT=${GLSL_SHADERS_C}"
            -P "${DVZ_EMBED_TEXT_RESOURCES_SCRIPT}"
        DEPENDS ${GLSL_TEXT_SOURCES} "${DVZ_EMBED_TEXT_RESOURCES_SCRIPT}"
        COMMENT "Embedding GLSL shaders into _glsl_shaders.c"
    )
    add_custom_target(datoviz_glsl_shaders DEPENDS "${GLSL_SHADERS_C}")
    set(DVZ_HAS_GLSL_SHADERS 1)
    set(DVZ_GLSL_SHADERS_C "${GLSL_SHADERS_C}")
else()
    set(DVZ_HAS_GLSL_SHADERS 0)
    set(DVZ_GLSL_SHADERS_C "")
endif()

# -------------------------------------------------------------------------------------------------
# WGSL embedding — browser/WebGPU-ready scene shader variants.
# -------------------------------------------------------------------------------------------------

set(WGSL_SOURCE_DIR "${PROJECT_SOURCE_DIR}/src/scene/shaders/wgsl")
if(EXISTS "${WGSL_SOURCE_DIR}")
    file(GLOB WGSL_SOURCES "${WGSL_SOURCE_DIR}/*.wgsl")
else()
    set(WGSL_SOURCES "")
endif()

if(WGSL_SOURCES)
    set(WGSL_SHADERS_C "${CMAKE_BINARY_DIR}/_wgsl_shaders.c")
    string(REPLACE ";" "\\;" _wgsl_list_escaped "${WGSL_SOURCES}")
    add_custom_command(
        OUTPUT  "${WGSL_SHADERS_C}"
        COMMAND "${CMAKE_COMMAND}"
            "-DFILES=${_wgsl_list_escaped}"
            "-DPREFIX=wgsl"
            "-DOUTPUT=${WGSL_SHADERS_C}"
            -P "${DVZ_EMBED_TEXT_RESOURCES_SCRIPT}"
        DEPENDS ${WGSL_SOURCES} "${DVZ_EMBED_TEXT_RESOURCES_SCRIPT}"
        COMMENT "Embedding WGSL shaders into _wgsl_shaders.c"
    )
    add_custom_target(datoviz_wgsl_shaders DEPENDS "${WGSL_SHADERS_C}")
    set(DVZ_HAS_WGSL_SHADERS 1)
    set(DVZ_WGSL_SHADERS_C "${WGSL_SHADERS_C}")
else()
    set(DVZ_HAS_WGSL_SHADERS 0)
    set(DVZ_WGSL_SHADERS_C "")
endif()

# -------------------------------------------------------------------------------------------------
# Font embedding — compact defaults for scene text.
# -------------------------------------------------------------------------------------------------

set(DVZ_HAS_EMBEDDED_FONTS 0)
set(DVZ_FONTS_C "")
if(DVZ_BUILD_SCENE OR DVZ_BUILD_GUI)
    set(FONT_SOURCES
        "${PROJECT_SOURCE_DIR}/assets/runtime/fonts/SourceSans3-Regular.ttf"
        "${PROJECT_SOURCE_DIR}/assets/runtime/fonts/SourceSans3-Bold.ttf"
        "${PROJECT_SOURCE_DIR}/assets/runtime/fonts/SourceSans3-It.ttf"
        "${PROJECT_SOURCE_DIR}/assets/runtime/fonts/SourceSans3-BoldIt.ttf"
        "${PROJECT_SOURCE_DIR}/assets/runtime/fonts/SourceCodePro-Regular.ttf"
        "${PROJECT_SOURCE_DIR}/assets/runtime/fonts/NotoSansMath-Regular.ttf"
    )
    set(FONTS_C "${CMAKE_BINARY_DIR}/_fonts.c")
    string(REPLACE ";" "\\;" _font_list_escaped "${FONT_SOURCES}")
    add_custom_command(
        OUTPUT  "${FONTS_C}"
        COMMAND "${CMAKE_COMMAND}"
            "-DFILES=${_font_list_escaped}"
            "-DPREFIX=font"
            "-DOUTPUT=${FONTS_C}"
            -P "${DVZ_EMBED_RESOURCES_SCRIPT}"
        DEPENDS ${FONT_SOURCES} "${DVZ_EMBED_RESOURCES_SCRIPT}"
        COMMENT "Embedding scene fonts into _fonts.c"
    )
    add_custom_target(datoviz_fonts DEPENDS "${FONTS_C}")
    set(DVZ_HAS_EMBEDDED_FONTS 1)
    set(DVZ_FONTS_C "${FONTS_C}")
endif()

# Add subdirectories
add_subdirectory(src)

if(DVZ_BUILD_TESTING)
    add_subdirectory(testing)
endif()

if(DVZ_BUILD_EXAMPLES)
    if(DVZ_BUILD_CANVAS)
        add_subdirectory(examples/c)
    endif()

    if(DVZ_BUILD_APP)
        add_subdirectory(examples/qt)
    endif()
endif()

get_property(_dvz_targets GLOBAL PROPERTY DVZ_ALL_TARGETS)
list(REMOVE_DUPLICATES _dvz_targets)
get_property(_dvz_compile_definitions GLOBAL PROPERTY DVZ_COMPILE_DEFINITIONS)

foreach(_dvz_target ${_dvz_targets})
    if(TARGET ${_dvz_target})
        get_target_property(_dvz_target_type ${_dvz_target} TYPE)

        if(DVZ_COMMON_COMPILE_OPTIONS)
            target_compile_options(${_dvz_target} PRIVATE
                $<$<NOT:$<COMPILE_LANGUAGE:CUDA>>:${DVZ_COMMON_COMPILE_OPTIONS}>
            )
        endif()

        if(DVZ_COMPILE_OPTIONS_C)
            target_compile_options(${_dvz_target} PRIVATE $<$<COMPILE_LANGUAGE:C>:${DVZ_COMPILE_OPTIONS_C}>)
        endif()

        if(DVZ_COMPILE_OPTIONS_CXX)
            target_compile_options(${_dvz_target} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${DVZ_COMPILE_OPTIONS_CXX}>)
        endif()

        if(DVZ_COMPILE_OPTIONS_OBJC)
            target_compile_options(${_dvz_target} PRIVATE $<$<COMPILE_LANGUAGE:OBJC>:${DVZ_COMPILE_OPTIONS_OBJC}>)
        endif()

        if(_dvz_compile_definitions)
            target_compile_definitions(${_dvz_target} PRIVATE ${_dvz_compile_definitions})
        endif()

        if(DVZ_SANITIZER_FLAGS AND _dvz_target_type AND
           NOT _dvz_target_type STREQUAL "INTERFACE_LIBRARY")
            target_compile_options(${_dvz_target} PRIVATE
                $<$<AND:$<CONFIG:Debug>,$<NOT:$<COMPILE_LANGUAGE:CUDA>>>:${DVZ_SANITIZER_FLAGS}>
            )

            if(NOT _dvz_target_type STREQUAL "OBJECT_LIBRARY" AND NOT CC_MSVC)
                target_link_options(${_dvz_target} PRIVATE
                    $<$<CONFIG:Debug>:${DVZ_SANITIZER_FLAGS}>
                )
            endif()
        endif()
    endif()
endforeach()

if(CMAKE_C_COMPILER_ID MATCHES "MSVC" OR CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(_dvz_external_nowarn "/W0")
else()
    set(_dvz_external_nowarn "-w")
endif()

file(GLOB_RECURSE DVZ_EXTERNAL_SOURCES CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/external/*.c"
    "${PROJECT_SOURCE_DIR}/external/*.cc"
    "${PROJECT_SOURCE_DIR}/external/*.cpp"
    "${PROJECT_SOURCE_DIR}/external/*.cxx"
    "${PROJECT_SOURCE_DIR}/external/*.m"
    "${PROJECT_SOURCE_DIR}/external/*.mm"
)

if(DVZ_EXTERNAL_SOURCES AND _dvz_external_nowarn)
    set_property(SOURCE ${DVZ_EXTERNAL_SOURCES} APPEND_STRING PROPERTY COMPILE_FLAGS " ${_dvz_external_nowarn}")
endif()

if(DVZ_ENABLE_COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Code coverage instrumentation enabled")
        get_property(_dvz_targets GLOBAL PROPERTY DVZ_ALL_TARGETS)
        list(REMOVE_DUPLICATES _dvz_targets)

        set(_dvz_coverage_compile_flags -O0 -g --coverage)
        set(_dvz_coverage_link_flags --coverage)

        foreach(_dvz_target ${_dvz_targets})
            if(TARGET ${_dvz_target})
                get_target_property(_dvz_target_type ${_dvz_target} TYPE)

                if(_dvz_target_type AND NOT _dvz_target_type STREQUAL "INTERFACE_LIBRARY")
                    target_compile_options(${_dvz_target} PRIVATE ${_dvz_coverage_compile_flags})

                    if(NOT _dvz_target_type STREQUAL "OBJECT_LIBRARY")
                        target_link_options(${_dvz_target} PRIVATE ${_dvz_coverage_link_flags})
                    endif()
                endif()
            endif()
        endforeach()
    else()
        message(WARNING "DVZ_ENABLE_COVERAGE is ON but coverage flags are only supported with GCC or Clang")
    endif()
endif()

if(DVZ_INSTALL AND NOT CMAKE_SKIP_INSTALL_RULES)
    include(CMakePackageConfigHelpers)
    set(_dvz_package_dir "${CMAKE_CURRENT_BINARY_DIR}/cmake")
    set(DVZ_PKG_CONFIG_PUBLIC_CFLAGS "")

    if((DVZ_BUILD_VK OR DVZ_BUILD_CANVAS) AND NOT Vulkan_FOUND)
        find_package(Vulkan QUIET)
    endif()

    set(_dvz_vulkan_public_include_dirs "")
    if(Vulkan_INCLUDE_DIRS)
        list(APPEND _dvz_vulkan_public_include_dirs ${Vulkan_INCLUDE_DIRS})
    endif()
    if(Vulkan_INCLUDE_DIR)
        list(APPEND _dvz_vulkan_public_include_dirs "${Vulkan_INCLUDE_DIR}")
    endif()

    if((DVZ_BUILD_VK OR DVZ_BUILD_CANVAS) AND Vulkan_FOUND AND _dvz_vulkan_public_include_dirs)
        foreach(_dvz_vulkan_include_dir IN LISTS _dvz_vulkan_public_include_dirs)
            list(APPEND DVZ_PKG_CONFIG_PUBLIC_CFLAGS "-I${_dvz_vulkan_include_dir}")
        endforeach()
        list(REMOVE_DUPLICATES DVZ_PKG_CONFIG_PUBLIC_CFLAGS)
        string(JOIN " " DVZ_PKG_CONFIG_PUBLIC_CFLAGS ${DVZ_PKG_CONFIG_PUBLIC_CFLAGS})
    endif()

    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/packages/DatovizConfig.cmake.in"
        "${_dvz_package_dir}/DatovizConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/datoviz"
    )
    configure_package_config_file(
        "${PROJECT_SOURCE_DIR}/cmake/packages/DatovizConfig.cmake.in"
        "${_dvz_package_dir}/datovizConfig.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/datoviz"
    )

    write_basic_package_version_file(
        "${_dvz_package_dir}/DatovizConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )
    write_basic_package_version_file(
        "${_dvz_package_dir}/datovizConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(EXPORT DatovizTargets
        FILE DatovizTargets.cmake
        NAMESPACE datoviz::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/datoviz"
    )

    install(FILES
        "${_dvz_package_dir}/DatovizConfig.cmake"
        "${_dvz_package_dir}/DatovizConfigVersion.cmake"
        "${_dvz_package_dir}/datovizConfig.cmake"
        "${_dvz_package_dir}/datovizConfigVersion.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/datoviz"
    )

    configure_file(cmake/datoviz.pc.in datoviz.pc @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/datoviz.pc"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
    )

    set(_dvz_third_party_license_manifest
        "${PROJECT_SOURCE_DIR}/licenses/THIRD_PARTY_LICENSES.txt")
    if(NOT EXISTS "${_dvz_third_party_license_manifest}")
        message(FATAL_ERROR
            "Datoviz third-party license manifest is missing: ${_dvz_third_party_license_manifest}")
    endif()
    file(STRINGS "${_dvz_third_party_license_manifest}" _dvz_third_party_license_paths)
    foreach(_dvz_third_party_license_path IN LISTS _dvz_third_party_license_paths)
        string(STRIP "${_dvz_third_party_license_path}" _dvz_third_party_license_path)
        if(_dvz_third_party_license_path STREQUAL "" OR _dvz_third_party_license_path MATCHES "^#")
            continue()
        endif()
        if(IS_ABSOLUTE "${_dvz_third_party_license_path}" OR _dvz_third_party_license_path MATCHES "(^|/)\\.\\.(/|$)")
            message(FATAL_ERROR
                "Unsafe third-party license manifest path: ${_dvz_third_party_license_path}")
        endif()
        set(_dvz_third_party_license_source
            "${PROJECT_SOURCE_DIR}/${_dvz_third_party_license_path}")
        if(NOT EXISTS "${_dvz_third_party_license_source}")
            message(FATAL_ERROR "Third-party license authority is missing: ${_dvz_third_party_license_path}")
        endif()
        get_filename_component(_dvz_third_party_license_directory
            "${_dvz_third_party_license_path}" DIRECTORY)
        install(FILES "${_dvz_third_party_license_source}"
            DESTINATION "${CMAKE_INSTALL_DOCDIR}/licenses/${_dvz_third_party_license_directory}"
        )
    endforeach()
    install(FILES "${PROJECT_SOURCE_DIR}/LICENSE"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}/licenses"
    )
    install(FILES
        "${PROJECT_SOURCE_DIR}/licenses/THIRD_PARTY_NOTICES.md"
        "${PROJECT_SOURCE_DIR}/licenses/THIRD_PARTY_LICENSES.txt"
        DESTINATION "${CMAKE_INSTALL_DOCDIR}/licenses/licenses"
    )
endif()

# Configuration summary.
message(STATUS "")
message(STATUS "---------------- Datoviz build configuration ----------------")
message(STATUS "Generator: ${CMAKE_GENERATOR}")

if(CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Configured build types: ${CMAKE_CONFIGURATION_TYPES}")
else()
    message(STATUS "Active build type: ${CMAKE_BUILD_TYPE}")
endif()

message(STATUS "Threading backend: ${DVZ_THREADING_TARGET}")
message(STATUS "DVZ_ENABLE_ASAN_IN_DEBUG: ${DVZ_ENABLE_ASAN_IN_DEBUG}")
message(STATUS "DVZ_ENABLE_COVERAGE: ${DVZ_ENABLE_COVERAGE}")
message(STATUS "DVZ_USE_MIMALLOC_RELEASE_DEFAULT: ${DVZ_USE_MIMALLOC_RELEASE_DEFAULT}")
message(STATUS "DVZ_ENABLE_CUDA: ${DVZ_ENABLE_CUDA}")
message(STATUS "DVZ_HAS_CUDA: ${DVZ_HAS_CUDA}")
message(STATUS "DVZ_HAS_NVENC: ${DVZ_HAS_NVENC}")
message(STATUS "DVZ_WITH_GLFW: ${DVZ_WITH_GLFW}")
message(STATUS "DVZ_HAS_GLFW: ${DVZ_HAS_GLFW}")
message(STATUS "DVZ_CGLM_SOURCE: ${DVZ_CGLM_SOURCE} (${DVZ_CGLM_SOURCE_RESOLVED})")
message(STATUS "DVZ_HAS_CGLM: ${DVZ_HAS_CGLM}")
message(STATUS "DVZ_KVAZAAR_SOURCE: ${DVZ_KVAZAAR_SOURCE} (${DVZ_KVAZAAR_SOURCE_RESOLVED})")
message(STATUS "DVZ_ENABLE_KVAZAAR: ${DVZ_ENABLE_KVAZAAR}")
message(STATUS "DVZ_HAS_KVZ: ${DVZ_HAS_KVZ}")
message(STATUS "DVZ_WITH_ZLIB: ${DVZ_WITH_ZLIB}")
message(STATUS "DVZ_HAS_ZLIB: ${DVZ_HAS_ZLIB}")
message(STATUS "DVZ_MIMALLOC_SOURCE: ${DVZ_MIMALLOC_SOURCE} (${DVZ_MIMALLOC_SOURCE_RESOLVED})")
message(STATUS "DVZ_WITH_MIMALLOC (detected): ${DVZ_WITH_MIMALLOC}")
message(STATUS "DVZ_BUILD_CORE: ${DVZ_BUILD_CORE}")
message(STATUS "DVZ_BUILD_CONTROLLER: ${DVZ_BUILD_CONTROLLER}")
message(STATUS "DVZ_BUILD_VK: ${DVZ_BUILD_VK}")
message(STATUS "DVZ_BUILD_CANVAS: ${DVZ_BUILD_CANVAS}")
message(STATUS "DVZ_BUILD_DRP2: ${DVZ_BUILD_DRP2}")
message(STATUS "DVZ_BUILD_WEBGPU: ${DVZ_BUILD_WEBGPU}")
message(STATUS "DVZ_BUILD_SCENE: ${DVZ_BUILD_SCENE}")
message(STATUS "DVZ_BUILD_APP: ${DVZ_BUILD_APP}")
message(STATUS "DVZ_BUILD_GUI: ${DVZ_BUILD_GUI}")
message(STATUS "DVZ_BUILD_TESTING: ${DVZ_BUILD_TESTING}")
message(STATUS "DVZ_BUILD_EXAMPLES: ${DVZ_BUILD_EXAMPLES}")
message(STATUS "DVZ_BUILD_IMPLOT_EXAMPLE: ${DVZ_BUILD_IMPLOT_EXAMPLE}")
message(STATUS "DVZ_INSTALL: ${DVZ_INSTALL}")
message(STATUS "DVZ_VENDORED_DEPS: ${DVZ_VENDORED_DEPS}")
message(STATUS "DVZ_ENABLE_QT_BRIDGE: ${DVZ_ENABLE_QT_BRIDGE}")
message(STATUS "DVZ_HAS_QT_BRIDGE: ${DVZ_HAS_QT_BRIDGE}")
message(STATUS "GLSLC: ${GLSLC}")
message(STATUS "DVZ_VALIDATE_SPIRV: ${DVZ_VALIDATE_SPIRV}")
message(STATUS "DVZ_SPIRV_VAL_EXECUTABLE: ${DVZ_SPIRV_VAL_EXECUTABLE}")

if(DEFINED DVZ_USE_VALIDATION)
    message(STATUS "DVZ_USE_VALIDATION: ${DVZ_USE_VALIDATION} (default contexts remain quiet in Release configs)")
endif()

message(STATUS "-------------------------------------------------------------")
message(STATUS "")

if(DVZ_INSTALL AND NOT CMAKE_SKIP_INSTALL_RULES)
    # Install headers and library
    install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

    if(DVZ_BUILD_VK OR DVZ_BUILD_CANVAS)
        install(FILES
            ${PROJECT_SOURCE_DIR}/external/vk_mem_alloc.h
            ${PROJECT_SOURCE_DIR}/external/volk/volk.h
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        )
    endif()

    if(DVZ_BUILD_GUI)
        install(FILES
            ${PROJECT_SOURCE_DIR}/external/cimgui/cimgui.h
            DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
        )
    endif()
endif()
