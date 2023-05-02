set(CMAKE_VERBOSE_MAKEFILE OFF)

cmake_minimum_required(VERSION 3.10)
enable_testing()
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include("cmake/colormsg.cmake")
include(FetchContent)

set(CMAKE_C_STANDARD)
set(CMAKE_C_STANDARD_REQUIRED True)


# Options
option(DATOVIZ_WITH_VULKAN_SDK "Use the Vulkan SDK, otherwise use bundled Vulkan toolchain" ON)
option(DATOVIZ_WITH_GLSLANG "Build Datoviz with glslang support" OFF)  # TODO: remove?
option(DATOVIZ_WITH_PNG "Build Datoviz with PNG support" ON)
option(DATOVIZ_WITH_FFMPEG "Build Datoviz with FFMPEG support" ON)
option(DATOVIZ_WITH_SWIFTSHADER "Build Datoviz with Swiftshader support" OFF)
option(DATOVIZ_WITH_CLI "Build Datoviz command-line interface with tests and demos" ON)

# Swiftshader boolean
# Flag to compile with support for swiftshader, which makes mandatory the use of offscreen
# rendering. All features related to swapchains etc are forbidden.
set(SWIFTSHADER 0)
if(DATOVIZ_WITH_SWIFTSHADER)
    set(SWIFTSHADER 1)
endif()

# Define the project
set(DATOVIZ_VERSION 0.1.0)
project(datoviz VERSION ${DATOVIZ_VERSION} DESCRIPTION "datoviz")

# DEBUG/RELEASE
set(DEBUG 1)
set(CMAKE_BUILD_TYPE Debug)


# -------------------------------------------------------------------------------------------------
# Dependencies
# -------------------------------------------------------------------------------------------------

# cglm
FetchContent_Declare(
    cglm
    GIT_REPOSITORY  https://github.com/recp/cglm/
    # GIT_TAG         v0.8.3
)
FetchContent_MakeAvailable(cglm)

# glfw3
#set(GLFW_LIBRARY_TYPE SHARED CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY  https://github.com/glfw/glfw/
    # GIT_TAG         3.3.7
)
FetchContent_MakeAvailable(glfw)

# Vulkan
if (DATOVIZ_WITH_VULKAN_SDK)
    find_package(Vulkan)
endif()
set(HAS_GLSLANG 0)
if (Vulkan_FOUND)
    message(STATUS "Found Vulkan")

    find_program(GLSLC glslc)
    if (NOT GLSLC)
        message(FATAL_ERROR "-- glslc executable NOT found")
    endif()

    # glslang
    # TODO: remove?
    set(glslangValidator "")
    if (DATOVIZ_WITH_GLSLANG)
        FetchContent_Declare(
            glslang
            GIT_REPOSITORY https://github.com/KhronosGroup/glslang
            GIT_TAG        11.6.0
        )
        FetchContent_MakeAvailable(glslang)
        set(HAS_GLSLANG 1)
    endif()
else()
    message(
        WARNING "-- Could not find Vulkan SDK, fallback to manual download/build of Vulkan loader")

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
        # GIT_REPOSITORY  https://github.com/KhronosGroup/SPIRV-Tools.git
        # GIT_TAG         master # v2021.3
        # TEMPORARY FIX https://github.com/KhronosGroup/SPIRV-Tools/issues/4565
        GIT_REPOSITORY  https://github.com/jaebaek/SPIRV-Tools.git
        GIT_TAG         fix_ci_failure
    )
    FetchContent_MakeAvailable(SPIRV-Tools)

    # glslang
    FetchContent_Declare(
        glslang
        GIT_REPOSITORY https://github.com/KhronosGroup/glslang
        GIT_TAG        master # 11.6.0
    )
    FetchContent_MakeAvailable(glslang)
    if (DATOVIZ_WITH_GLSLANG)
        set(HAS_GLSLANG 1)
    endif()

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

    set(GLSLC glslc_exe)

endif()


# -------------------------------------------------------------------------------------------------
# Compiler options
# -------------------------------------------------------------------------------------------------

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
        -Wshadow -Wsign-conversion -Wstrict-overflow=5 -Wundef -Wno-unused-parameter \
        -Wno-unused-function")
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


# -------------------------------------------------------------------------------------------------
# Include and linking directories
# -------------------------------------------------------------------------------------------------

set(INCL_DIRS
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/imgui
    ${PROJECT_SOURCE_DIR}/external/imgui/backends
    ${PROJECT_SOURCE_DIR}/include/
    ${Vulkan_INCLUDE_DIRS}
)
set(LINK_LIBS m glfw cglm pthread)
if(DATOVIZ_WITH_VULKAN_SDK)
    set(LINK_LIBS ${LINK_LIBS} Vulkan::Vulkan)
else()
    set(LINK_LIBS ${LINK_LIBS} vulkan)
endif()
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
    find_library(FFMPEG_LIBSWRESAMPLE NAMES NAMES swresample libswresample
                 PATH_SUFFIXES ffmpeg/libswresample)
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


# -------------------------------------------------------------------------------------------------
# Build variables and macros
# -------------------------------------------------------------------------------------------------

set(DATA_DIR "${PROJECT_SOURCE_DIR}/data")
set(SPIRV_DIR ${PROJECT_BINARY_DIR}/spirv)
set(ARTIFACTS_DIR "${PROJECT_BINARY_DIR}/artifacts")

file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR})
file(MAKE_DIRECTORY ${SPIRV_DIR})
file(MAKE_DIRECTORY "${ARTIFACTS_DIR}")

# Pass definitions
set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS}
    LOG_USE_COLOR
    ENABLE_VALIDATION_LAYERS=1
    ROOT_DIR=\"${PROJECT_SOURCE_DIR}\"
    DATA_DIR=\"${DATA_DIR}\"
    SPIRV_DIR=\"${SPIRV_DIR}\"
    ARTIFACTS_DIR=\"${ARTIFACTS_DIR}\"

    HAS_FFMPEG=${HAS_FFMPEG}
    HAS_PNG=${HAS_PNG}
    HAS_GLSLANG=${HAS_GLSLANG}

    OS_MACOS=${OS_MACOS}
    OS_WIN32=${OS_WIN32}

    MSVC=${CC_MSVC}
    GCC=${CC_GCC}
    CLANG=${CC_CLANG}

    SWIFTSHADER=${SWIFTSHADER}
)


# -------------------------------------------------------------------------------------------------
# Shaders compilation
# -------------------------------------------------------------------------------------------------

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
        COMMAND ${GLSLC}  # this is a target declared by the glslc fetch module
            -o "${shader_output}" ${shader_source}
            -I "${PROJECT_SOURCE_DIR}/include/datoviz/glsl"
        DEPENDS ${shader_source} ${glslang}
        IMPLICIT_DEPENDS ${shader_source} ${glslang}
        )
    list(APPEND shader_outputs ${shader_output})
endforeach()
add_custom_target(shaders_spirv DEPENDS ${shader_outputs})

# NOTE: Only include graphics shaders in the embed resources files.
# file(GLOB embed_spirv "${SPIRV_DIR}/graphics_*.spv")
# message(${embed_spirv})
set(path_shadersc "${PROJECT_BINARY_DIR}/_shaders.c")
add_custom_command(
    OUTPUT ${path_shadersc}
    COMMAND ${CMAKE_COMMAND}
    -D DIR="${SPIRV_DIR}/*"
    -D PREFIX="shader"
    -D OUTPUT=${path_shadersc}
    -P "${PROJECT_SOURCE_DIR}/cmake/embed_resources.cmake"
    DEPENDS shaders_spirv ${shader_sources}
    IMPLICIT_DEPENDS shaders_spirv ${shader_sources}
)
add_custom_target(shaders DEPENDS ${path_shadersc})


# -------------------------------------------------------------------------------------------------
# Other binary resources
# -------------------------------------------------------------------------------------------------

# Color texture.
set(path_colortex "${PROJECT_BINARY_DIR}/_colortex.c")
add_custom_command(
    OUTPUT ${path_colortex}
    COMMAND ${CMAKE_COMMAND}
    -D DIR="${PROJECT_SOURCE_DIR}/data/textures/color_texture.img"
    -D PREFIX="texture"
    -D OUTPUT=${path_colortex}
    -P "${PROJECT_SOURCE_DIR}/cmake/embed_resources.cmake"
)

# Fonts.
set(path_fonts "${PROJECT_BINARY_DIR}/_fonts.c")
set(font_files
    "${PROJECT_SOURCE_DIR}/data/textures/font_inconsolata.png"
    "${PROJECT_SOURCE_DIR}/data/fonts/Roboto-Medium.ttf"
    "${PROJECT_SOURCE_DIR}/data/fonts/fontawesome-webfont.ttf")
add_custom_command(
    OUTPUT ${path_fonts}
    COMMAND ${CMAKE_COMMAND}
    -D DIR="${font_files}"
    -D PREFIX="font"
    -D OUTPUT=${path_fonts}
    -P "${PROJECT_SOURCE_DIR}/cmake/embed_resources.cmake"
)


# -------------------------------------------------------------------------------------------------
# Datoviz build
# -------------------------------------------------------------------------------------------------

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
add_library(datoviz SHARED ${sources} ${path_shadersc} ${path_colortex} ${path_fonts})

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
# Datoviz CLI
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


# -------------------------------------------------------------------------------------------------
# Test build
# -------------------------------------------------------------------------------------------------

if (DATOVIZ_WITH_CLI)
    file(GLOB cli "cli/*.c" "tests/*.c*")
    add_executable(datovizcli ${cli})
    target_datoviz(datovizcli)
    set_target_properties(datovizcli PROPERTIES OUTPUT_NAME "datoviz")
    add_test(NAME datoviz_test COMMAND datovizcli test)
endif()


# -------------------------------------------------------------------------------------------------
# Leftovers
# -------------------------------------------------------------------------------------------------

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


# experimental
# add_executable(freetype_example experimental/freetype-example.c)
# target_include_directories(freetype_example PUBLIC ${INCL_DIRS})
# target_link_libraries(freetype_example freetype -lm)


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


# if (NOT CMAKE_BUILD_TYPE)
#    set(CMAKE_BUILD_TYPE Debug)
# endif()
# if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
#     set(DEBUG 1)
# endif ()


# option(DATOVIZ_WITH_VNC "Build Datoviz with VNC support" OFF)
# option(DATOVIZ_WITH_QT "Build Datoviz with QT support" OFF)
# option(DATOVIZ_WITH_ASSIMP "Build Datoviz with ASSIMP support" OFF)
# option(DATOVIZ_WITH_FREETYPE "Build Datoviz with Freetype support" OFF)
# option(DATOVIZ_WITH_EXAMPLES "Build Datoviz (old) examples" OFF)
# option(DATOVIZ_WITH_CYTHON "Build Cython bindings" OFF)
