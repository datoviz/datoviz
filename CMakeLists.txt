set(CMAKE_VERBOSE_MAKEFILE OFF)

cmake_minimum_required(VERSION 3.10)
enable_testing()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include("cmake/colormsg.cmake")
include(FetchContent)

set(CMAKE_C_STANDARD)
set(CMAKE_C_STANDARD_REQUIRED True)


# Options
# option(DATOVIZ_WITH_VNC "Build Datoviz with VNC support" OFF)
# option(DATOVIZ_WITH_QT "Build Datoviz with QT support" OFF)
# option(DATOVIZ_WITH_ASSIMP "Build Datoviz with ASSIMP support" OFF)
# option(DATOVIZ_WITH_FREETYPE "Build Datoviz with Freetype support" OFF)
option(DATOVIZ_WITH_PNG "Build Datoviz with PNG support" ON)
option(DATOVIZ_WITH_FFMPEG "Build Datoviz with FFMPEG support" ON)
option(DATOVIZ_WITH_GLSLANG "Build Datoviz with glslang support" OFF)

option(DATOVIZ_WITH_CLI "Build Datoviz command-line interface with tests and demos" ON)
# option(DATOVIZ_WITH_EXAMPLES "Build Datoviz (old) examples" OFF)
# option(DATOVIZ_WITH_CYTHON "Build Cython bindings" OFF)


# Define the project
set(DATOVIZ_VERSION 0.0.1)
project(datoviz VERSION ${DATOVIZ_VERSION} DESCRIPTION "datoviz")


# DEBUG
set(DEBUG 1)
set(CMAKE_BUILD_TYPE Debug)
# if (NOT CMAKE_BUILD_TYPE)
#    set(CMAKE_BUILD_TYPE Debug)
# endif()
# if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
#     set(DEBUG 1)
# endif ()


# Vulkan dependency
find_package(Vulkan)
if (Vulkan_FOUND)
    message(STATUS "Found Vulkan")
else()
    message(FATAL_ERROR "-- Could NOT find Vulkan")
endif()

# # cglm dependency: download from GitHub
# FetchContent_Declare(
#   cglm
#   GIT_REPOSITORY https://github.com/recp/cglm
#   GIT_TAG        v0.7.8
# )
# FetchContent_MakeAvailable(cglm)
add_subdirectory(external/cglm/)

# glfw3 dependency on Windows
if (WIN32 OR APPLE)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw/
    )
    FetchContent_MakeAvailable(glfw)
endif()


set(HAS_GLSLANG 0)
if (DATOVIZ_WITH_GLSLANG)
    FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang
    GIT_TAG        master-tot
    )
    FetchContent_MakeAvailable(glslang)
    set(HAS_GLSLANG 1)
endif()


# HACK: ensure correct version of glfw3 >= 3.3
set(GLFW_GT_33 1)
if(UNIX AND NOT APPLE)
    find_package(glfw3)
    if (${glfw3_VERSION} VERSION_GREATER "3.3")
    else()
    set(GLFW_GT_33 0)
    endif()
endif()


set(CC_GCC 0)
set(CC_CLANG 0)
set(CC_MSVC 0)

if (MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    set(CC_MSVC 1)
else ()
    # NOTE: need to remove -pg (gprof profiling) in RELEASE mode?
    set(COMMON_FLAGS "-g -m64 -pedantic -Wall -Wextra -Werror=vla \
        -Wcast-align -Wcast-qual -Wredundant-decls -Wswitch-default -Wdisabled-optimization \
        -Wformat=2 -Winit-self -Wno-missing-field-initializers -Wno-unused-result \
        -Wmissing-declarations -Wmissing-include-dirs -Wno-format-nonliteral \
        -Wshadow -Wsign-conversion -Wstrict-overflow=5 -Wundef -Wno-unused-parameter -Wno-unused-function")
    # GCC-specific
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    set(COMMON_FLAGS "${COMMON_FLAGS} -Wlogical-op -fdiagnostics-color=always")
    if (NOT WIN32)
    set(COMMON_FLAGS "${COMMON_FLAGS} -pg")
    endif()
    set(CC_GCC 1)
    # Clang-specific
    elseif ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
    set(COMMON_FLAGS "${COMMON_FLAGS} -Wno-unused-parameter -fcolor-diagnostics -Wno-missing-braces")
    set(CC_CLANG 1)
    endif()

    # The following seems to be required for nanosleep()
    if ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
    else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -Wmissing-prototypes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ${COMMON_FLAGS}")
endif()

set(COMMON_FLAGS_VISIBLE "${COMMON_FLAGS}")
set(COMMON_FLAGS "${COMMON_FLAGS} -fvisibility=hidden")


set(INCL_DIRS ${CMAKE_SOURCE_DIR}/external ${CMAKE_SOURCE_DIR}/external/imgui ${CMAKE_SOURCE_DIR}/external/imgui/backends ${CMAKE_SOURCE_DIR}/include/)
set(LINK_LIBS m glfw cglm Vulkan::Vulkan pthread)
if(NOT ${WIN32})
    set(LINK_LIBS dl ${LINK_LIBS})
endif()

if (DATOVIZ_WITH_GLSLANG)
    set(INCL_DIRS ${INCL_DIRS} ${glslang_SOURCE_DIR})
    set(LINK_LIBS ${LINK_LIBS} glslang SPIRV glslang-default-resource-limits)
endif()

# macOS-specific
set(OS_MACOS 0)
set(OS_WIN32 0)
if(${APPLE})
    set(INCL_DIRS ${INCL_DIRS} /usr/local/include)
    link_directories(/usr/local/lib)
    set(OS_MACOS 1)
# Windows-specific
elseif(${WIN32})
    set(INCL_DIRS ${INCL_DIRS} $ENV{VULKAN_SDK}/include $ENV{CGLM_INCLUDE})
    find_path(MINGW_DIR NAMES "gcc.exe")
    link_directories($ENV{CGLM_LIB} $ENV{VULKAN_SDK}\\Lib $ENV{VULKAN_SDK}\\Bin ${MINGW_DIR})
    set(OS_WIN32 1)
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
# Optional dependencies
# -------------------------------------------------------------------------------------------------

# Optional PNG support
set(HAS_PNG 0)
if(DATOVIZ_WITH_PNG)
    find_package(PNG)
    if(PNG_FOUND)
        set(HAS_PNG 1)
        set(INCL_DIRS ${INCL_DIRS} ${PNG_INCLUDE_DIRS})
        set(LINK_LIBS ${LINK_LIBS} ${PNG_LIBRARIES})
    endif()
endif()


# Optional FFmpeg support
set(HAS_FFMPEG 0)
if(DATOVIZ_WITH_FFMPEG)
    find_path(FFMPEG_INCLUDE_DIRS
        libavcodec/avcodec.h libavfilter/avfilter.h libavformat/avformat.h
        libavutil/avutil.h libswscale/swscale.h
    )
    find_library(FFMPEG_LIBAVCODEC NAMES avcodec libavcodec PATH_SUFFIXES ffmpeg/libavcodec)
    find_library(FFMPEG_LIBAVFILTER NAMES avfilter libavfilter PATH_SUFFIXES ffmpeg/libavfilter)
    find_library(FFMPEG_LIBAVFORMAT NAMES avformat libavformat PATH_SUFFIXES ffmpeg/libavformat)
    find_library(FFMPEG_LIBAVUTIL NAMES avutil libavutil PATH_SUFFIXES ffmpeg/libavutil)
    find_library(FFMPEG_LIBSWSCALE NAMES swscale libswscale PATH_SUFFIXES ffmpeg/libswscale)
    find_library(FFMPEG_LIBSWRESAMPLE NAMES NAMES swresample libswresample PATH_SUFFIXES ffmpeg/libswresample)
    if((
        FFMPEG_LIBAVCODEC AND
        FFMPEG_LIBAVFILTER AND
        FFMPEG_LIBAVFORMAT AND
        FFMPEG_LIBAVUTIL AND
        FFMPEG_LIBSWSCALE AND
        FFMPEG_LIBSWRESAMPLE))
        set(INCL_DIRS ${INCL_DIRS} ${FFMPEG_INCLUDE_DIRS})
        set(LINK_LIBS ${LINK_LIBS}
            ${FFMPEG_LIBAVCODEC}
            ${FFMPEG_LIBAVFILTER}
            ${FFMPEG_LIBAVFORMAT}
            ${FFMPEG_LIBAVUTIL}
            ${FFMPEG_LIBSWSCALE}
            ${FFMPEG_LIBSWRESAMPLE}
        )
        message(STATUS "Found FFMPEG")
        set(HAS_FFMPEG 1)
    else()
        message(WARNING "-- Could NOT find ffmpeg")
    endif()
endif()


# # Optional libVNCserver
# set(HAS_VNC 0)
# if(DATOVIZ_WITH_VNC)
#     find_path(LIBVNCSERVER_INCLUDE_DIRS rfb/rfb.h)
#     find_library(LIBVNCSERVER_LIB NAMES vncserver)
#     if((LIBVNCSERVER_INCLUDE_DIRS AND LIBVNCSERVER_LIB))
#         message(STATUS "Found VNCSERVER")
#         set(INCL_DIRS ${INCL_DIRS} ${LIBVNCSERVER_INCLUDE_DIRS})
#         set(LINK_LIBS ${LINK_LIBS} ${LIBVNCSERVER_LIB})
#         set(HAS_VNC 1)
#     else()
#         message(WARNING "-- Could NOT find vncserver")
#     endif()
# endif()


# # Optional Qt5
# set(HAS_QT 0)
# # HACK
# # if (${APPLE})
# #   set(DATOVIZ_WITH_QT 0)
# # endif()
# if(DATOVIZ_WITH_QT)
#     find_package(Qt5 5.12 COMPONENTS Widgets QUIET)
#     if(${Qt5_FOUND})
#         message(STATUS "Found Qt5 >= 5.12")
#         set(INCL_DIRS ${INCL_DIRS} ${Qt5})
#         set(LINK_LIBS ${LINK_LIBS} Qt5::Widgets)
#         set(HAS_QT 1)
#     else()
#         message(WARNING "-- Could NOT find Qt5")
#     endif()
# endif()


# # Optional ASSIMP
# set(HAS_ASSIMP 0)
# if(DATOVIZ_WITH_ASSIMP)
#     find_package(ASSIMP)
#     # if((ASSIMP_INCLUDE_DIRS AND ASSIMP_LIB))
#     if(ASSIMP_FOUND)
#         message(STATUS "Found ASSIMP")
#         set(INCL_DIRS ${INCL_DIRS} ${ASSIMP_INCLUDE_DIRS})
#         set(LINK_LIBS ${LINK_LIBS} ${ASSIMP_LIBRARIES})
#         set(HAS_ASSIMP 1)
#     else()
#         message(WARNING "-- Could NOT find ASSIMP")
#     endif()
# endif()


# # Optional Freetype
# set(HAS_FREETYPE 0)
# if(DATOVIZ_WITH_FREETYPE)
#     find_package(Freetype)
#     if(Freetype_FOUND)
#         message(STATUS "Found Freetype")
#         set(INCL_DIRS ${INCL_DIRS} ${FREETYPE_INCLUDE_DIRS})
#         set(LINK_LIBS ${LINK_LIBS} ${FREETYPE_LIBRARIES})
#         set(HAS_FREETYPE 1)
#     else()
#         message(WARNING "-- Could NOT find FREETYPE")
#     endif()
# endif()


# Pass definitions
set(DATA_DIR "${CMAKE_SOURCE_DIR}/data")
set(SPIRV_DIR ${CMAKE_BINARY_DIR}/spirv)
set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS}
    LOG_USE_COLOR
    ENABLE_VALIDATION_LAYERS=1
    ROOT_DIR=\"${CMAKE_SOURCE_DIR}\"
    DATA_DIR=\"${DATA_DIR}\"
    SPIRV_DIR=\"${SPIRV_DIR}\"
    ARTIFACTS_DIR=\"${CMAKE_SOURCE_DIR}/build/artifacts\"

    # HAS_VNC=${HAS_VNC}
    # HAS_QT=${HAS_QT}
    # HAS_ASSIMP=${HAS_ASSIMP}
    HAS_FFMPEG=${HAS_FFMPEG}
    HAS_PNG=${HAS_PNG}
    HAS_GLSLANG=${HAS_GLSLANG}

    OS_MACOS=${OS_MACOS}
    OS_WIN32=${OS_WIN32}

    MSVC=${CC_MSVC}
    GCC=${CC_GCC}
    CLANG=${CC_CLANG}

    GLFW_GT_33=${GLFW_GT_33}
)


file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR})
file(MAKE_DIRECTORY ${SPIRV_DIR})



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

# TODO: separate between builtin shaders and example/test visuals

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



# Color texture.
set(path_colortex "${CMAKE_BINARY_DIR}/_colortex.c")
add_custom_command(
    OUTPUT ${path_colortex}
    COMMAND ${CMAKE_COMMAND}
    -D DIR="${CMAKE_SOURCE_DIR}/data/textures/*.img"
    -D PREFIX="texture"
    -D OUTPUT=${path_colortex}
    -P "${CMAKE_SOURCE_DIR}/cmake/embed_resources.cmake"
)



# Define the datoviz library target
file(GLOB sources
    "include/*.h*"
    "src/*.h*" "src/*.c*"
    "external/*.h*" "external/*.c*"
    "external/imgui/*.h*" "external/imgui/*.c*"
    "external/imgui/backends/imgui_impl_glfw.*"
    "external/imgui/backends/imgui_impl_vulkan.*"
)

file(GLOB imgui_sources
    "external/imgui/*.h*" "external/imgui/*.c*"
    "external/imgui/backends/imgui_impl_glfw.*"
    "external/imgui/backends/imgui_impl_vulkan.*"
)

set_source_files_properties(${imgui_sources} PROPERTIES COMPILE_FLAGS -w)

add_library(datoviz SHARED ${sources} ${path_shadersc} ${path_colortex})


# -------------------------------------------------------------------------------------------------
# Datoviz build
# -------------------------------------------------------------------------------------------------

# Contains binaries automatically generated by the testing suite
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/build/artifacts")

set_target_properties(datoviz PROPERTIES
    VERSION ${DATOVIZ_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/datoviz/datoviz.h)
add_dependencies(datoviz shaders)

target_compile_definitions(datoviz PUBLIC ${COMPILE_DEFINITIONS})
target_include_directories(datoviz PUBLIC ${INCL_DIRS})
target_link_libraries(datoviz ${LINK_LIBS})
if(${WIN32})
    target_link_libraries(datoviz -static gcc stdc++ winpthread -dynamic)
endif()


# -------------------------------------------------------------------------------------------------
# Cython build
# -------------------------------------------------------------------------------------------------

# if (DATOVIZ_WITH_CYTHON)
#     add_subdirectory(bindings/cython/datoviz)
#     find_package(PythonExtensions)
#     find_package(Cython)
#     find_package(NumPy)
#     include_directories(${INCL_DIRS} ${PYTHON_INCLUDE_DIRS} ${NumPy_INCLUDE_DIRS})
#     add_cython_target(pydatoviz bindings/cython/datoviz/pydatoviz.pyx PY3 C)
#     add_library(pydatoviz MODULE ${pydatoviz})
#     python_extension_module(pydatoviz)
#     target_compile_options(pydatoviz PUBLIC -w)
#     # set_target_properties(
#     #     pydatoviz PROPERTIES
#     #     LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bindings/cython/datoviz/)

#     # WARNING: linking with {PYTHON_LIBRARIES} causes a Python segfault on macOS when importing
#     target_link_libraries(pydatoviz ${LINK_LIBS} datoviz)

#     # install(TARGETS pydatoviz LIBRARY DESTINATION datoviz)
#     # add_custom_target(pydatoviz ALL COMMAND ${CMAKE_COMMAND} -E create_symlink ${PROJECT_BINARY_DIR}/ bindings/cython/datoviz)
#     install(TARGETS pydatoviz LIBRARY DESTINATION src)
# endif()


# -------------------------------------------------------------------------------------------------
# Example build
# -------------------------------------------------------------------------------------------------

set(COMMON_FLAGS "${COMMON_FLAGS_VISIBLE}")

function(target_datoviz target_name)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
    endif()
    target_link_libraries(${target_name} datoviz)
endfunction()


set(DVZ_EXAMPLE $ENV{DVZ_EXAMPLE})

# Compilation function .
function(compile_example example_file)
    message("Compile example ${example_file}")
    get_filename_component(example_name ${example_file} NAME_WE)
    if(WIN32)
        add_executable(${example_name} WIN32 ${example_file})
    else()
        add_executable(${example_name} ${example_file})
    endif()
    target_datoviz(${example_name})
endfunction()


# if (DATOVIZ_WITH_EXAMPLES)
#     set(DVZ_EXAMPLE_PATH ${CMAKE_SOURCE_DIR}/examples/${DVZ_EXAMPLE})
#     if (${DVZ_EXAMPLE}xxx STREQUAL "xxx")
#         # Compile all examples
#         file(GLOB examples "examples/*.c" "examples/*.cpp")
#         foreach(example_file ${examples})
#             compile_example(${example_file})
#         endforeach()
#     elseif(EXISTS "${DVZ_EXAMPLE_PATH}.c")
#         # Or compile a single C example
#         compile_example("${DVZ_EXAMPLE_PATH}.c")
#     elseif(EXISTS "${DVZ_EXAMPLE_PATH}.cpp")
#         # Or compile a single C++ example
#         compile_example("${DVZ_EXAMPLE_PATH}.cpp")
#     elseif(NOT ${DVZ_EXAMPLE} STREQUAL "test")
#         message("WARNING: example '${DVZ_EXAMPLE}' not found")
#     endif()
# endif()



# -------------------------------------------------------------------------------------------------
# Test build
# -------------------------------------------------------------------------------------------------

if (DATOVIZ_WITH_CLI)
    # file(GLOB cli "cli/*.c" "cli/*.cpp" "cli/*.h" "cli/*.hpp" "tests/*.c*" "tests/*.h*")
    file(GLOB cli
        "cli/main.c"
        "tests/test_utils.c"
        "tests/test_vklite.c"
        "tests/test_context.c"
        "tests/test_canvas.c"
    )
    add_executable(datovizcli ${cli})
    target_datoviz(datovizcli)
    set_target_properties(datovizcli PROPERTIES OUTPUT_NAME "datoviz")
    add_test(NAME datoviz_test COMMAND datovizcli test)
endif()

# experimental
# add_executable(freetype_example experimental/freetype-example.c)
# target_include_directories(freetype_example PUBLIC ${INCL_DIRS})
# target_link_libraries(freetype_example freetype -lm)
