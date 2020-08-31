set(CMAKE_VERBOSE_MAKEFILE OFF)

cmake_minimum_required(VERSION 3.10)
enable_testing()
include("cmake/colormsg.cmake")
include(FetchContent)

set(CMAKE_C_STANDARD)
set(CMAKE_C_STANDARD_REQUIRED True)


# Options
option(VISKY_WITH_FFMPEG "Build Visky with FFMPEG support" ON)
option(VISKY_WITH_VNC "Build Visky with VNC support" ON)
option(VISKY_WITH_PNG "Build Visky with PNG support" ON)
option(VISKY_WITH_QT "Build Visky with QT support" ON)
option(VISKY_WITH_FREETYPE "Build Visky with Freetype support" ON)
option(VISKY_WITH_ASSIMP "Build Visky with ASSIMP support" OFF)


# Define the project
set(VISKY_VERSION 0.0.1)
project(visky VERSION ${VISKY_VERSION} DESCRIPTION "visky")

# Vulkan dependency
find_package(Vulkan)
if (Vulkan_FOUND)
    message(STATUS "Found Vulkan")
else()
    message(FATAL_ERROR "-- Could NOT find Vulkan")
endif()

# cglm dependency: download from GitHub
FetchContent_Declare(
  cglm
  GIT_REPOSITORY https://github.com/recp/cglm
  GIT_TAG        v0.7.4
)
FetchContent_MakeAvailable(cglm)

# glfw3 dependency on Windows
if (WIN32)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw/
    GIT_TAG        3.3.2
    )
    FetchContent_MakeAvailable(glfw)

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
        -Wformat=2 -Winit-self -Wno-missing-field-initializers \
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
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 ${COMMON_FLAGS} -Wmissing-prototypes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 ${COMMON_FLAGS}")
endif()

set(COMMON_FLAGS_VISIBLE "${COMMON_FLAGS}")
set(COMMON_FLAGS "${COMMON_FLAGS} -fvisibility=hidden")


# DEBUG/RELEASE
if (NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Debug)
endif()
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(DEBUG 1)
endif ()


set(INCL_DIRS ${CMAKE_SOURCE_DIR}/external ${CMAKE_SOURCE_DIR}/external/imgui ${CMAKE_SOURCE_DIR}/include/)
set(LINK_LIBS m dl glfw cglm Vulkan::Vulkan)

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
# Optional dependencies
# -------------------------------------------------------------------------------------------------

# Optional PNG support
set(HAS_PNG 0)
if(VISKY_WITH_PNG)
    find_package(PNG)
    if(PNG_FOUND)
        set(HAS_PNG 1)
        set(INCL_DIRS ${INCL_DIRS} ${PNG_INCLUDE_DIRS})
        set(LINK_LIBS ${LINK_LIBS} ${PNG_LIBRARIES})
    endif()
endif()


# Optional FFmpeg support
set(HAS_FFMPEG 0)
if(VISKY_WITH_FFMPEG)
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


# Optional libVNCserver
set(HAS_VNC 0)
if(VISKY_WITH_VNC)
    find_path(LIBVNCSERVER_INCLUDE_DIRS rfb/rfb.h)
    find_library(LIBVNCSERVER_LIB NAMES vncserver)
    if((LIBVNCSERVER_INCLUDE_DIRS AND LIBVNCSERVER_LIB))
        message(STATUS "Found VNCSERVER")
        set(INCL_DIRS ${INCL_DIRS} ${LIBVNCSERVER_INCLUDE_DIRS})
        set(LINK_LIBS ${LINK_LIBS} ${LIBVNCSERVER_LIB})
        set(HAS_VNC 1)
    else()
        message(WARNING "-- Could NOT find vncserver")
    endif()
endif()


# Optional Qt5
set(HAS_QT 0)
# HACK
if (${APPLE})
  set(VISKY_WITH_QT 0)
endif()
if(VISKY_WITH_QT)
    find_package(Qt5 COMPONENTS Widgets QUIET)
    if(${Qt5_FOUND})
        message(STATUS "Found QT5")
        set(INCL_DIRS ${INCL_DIRS} ${Qt5})
        set(LINK_LIBS ${LINK_LIBS} Qt5::Widgets)
        set(HAS_QT 1)
    else()
        message(WARNING "-- Could NOT find Qt5")
    endif()
endif()


# Optional ASSIMP
set(HAS_ASSIMP 0)
if(VISKY_WITH_ASSIMP)
    find_package(ASSIMP)
    # if((ASSIMP_INCLUDE_DIRS AND ASSIMP_LIB))
    if(ASSIMP_FOUND)
        message(STATUS "Found ASSIMP")
        set(INCL_DIRS ${INCL_DIRS} ${ASSIMP_INCLUDE_DIRS})
        set(LINK_LIBS ${LINK_LIBS} ${ASSIMP_LIBRARIES})
        set(HAS_ASSIMP 1)
    else()
        message(WARNING "-- Could NOT find ASSIMP")
    endif()
endif()


# Optional Freetype
set(HAS_FREETYPE 0)
if(VISKY_WITH_FREETYPE)
    find_package(Freetype)
    if(Freetype_FOUND)
        message(STATUS "Found Freetype")
        set(INCL_DIRS ${INCL_DIRS} ${FREETYPE_INCLUDE_DIRS})
        set(LINK_LIBS ${LINK_LIBS} ${FREETYPE_LIBRARIES})
        set(HAS_FREETYPE 1)
    else()
        message(WARNING "-- Could NOT find FREETYPE")
    endif()
endif()


# Pass definitions
set(DATA_DIR "${CMAKE_SOURCE_DIR}/data")
set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS}
    LOG_USE_COLOR
    ENABLE_VALIDATION_LAYERS=1
    ROOT_DIR=\"${CMAKE_SOURCE_DIR}\"
    DATA_DIR=\"${DATA_DIR}\"

    HAS_PNG=${HAS_PNG}
    HAS_FFMPEG=${HAS_FFMPEG}
    HAS_VNC=${HAS_VNC}
    HAS_QT=${HAS_QT}
    HAS_ASSIMP=${HAS_ASSIMP}

    OS_MACOS=${OS_MACOS}
    OS_WIN32=${OS_WIN32}

    MSVC=${CC_MSVC}
    GCC=${CC_GCC}
    CLANG=${CC_CLANG}
)


# set(QT5_INC /usr/local/qt5/include)
set(SPIRV_DIR ${CMAKE_SOURCE_DIR}/data/spirv/)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR})
file(MAKE_DIRECTORY ${SPIRV_DIR})



# -------------------------------------------------------------------------------------------------
# Shaders compilation
# -------------------------------------------------------------------------------------------------

find_program(GLSLC glslc)
file(GLOB shader_sources
    "src/shaders/*.vert"
    "src/shaders/*.geom"
    "src/shaders/*.comp"
    "src/shaders/*.frag"
    "examples/shaders/*.vert"
    "examples/shaders/*.geom"
    "examples/shaders/*.comp"
    "examples/shaders/*.frag"
)
foreach(shader_source ${shader_sources})
    get_filename_component(shader_name ${shader_source} NAME)
    # NOTE: there was a bug because the shader output was a relative path, not a full path
    # and cmake believed the output was not properly created by the custom command glslc.
    set(shader_output "${SPIRV_DIR}${shader_name}.spv")
    add_custom_command(
        OUTPUT ${shader_output}
        COMMAND ${GLSLC} -o "${shader_output}" ${shader_source}
        DEPENDS ${shader_source}
        IMPLICIT_DEPENDS ${shader_source}
        )
    list(APPEND shader_outputs ${shader_output})
endforeach()
add_custom_target(shaders DEPENDS ${shader_outputs})

# Define the visky library target
file(GLOB sources
    "include/*.h*"
    "src/*.h*" "src/*.c*"
    "external/*.h*" "external/*.c*"
    "external/imgui/*.h*" "external/imgui/*.c*"
)

add_library(visky SHARED ${sources})



# -------------------------------------------------------------------------------------------------
# Visky build
# -------------------------------------------------------------------------------------------------

set_target_properties(visky PROPERTIES
    VERSION ${VISKY_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/visky/visky.h)
add_dependencies(visky shaders)

target_compile_definitions(visky PUBLIC ${COMPILE_DEFINITIONS})
target_include_directories(visky PUBLIC ${INCL_DIRS})
target_link_libraries(visky ${LINK_LIBS})



# -------------------------------------------------------------------------------------------------
# Example build
# -------------------------------------------------------------------------------------------------

set(COMMON_FLAGS "${COMMON_FLAGS_VISIBLE}")

function(target_visky target_name)
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        add_compile_options(-fcolor-diagnostics)
    endif()
    target_link_libraries(${target_name} visky)
endfunction()


set(VKY_EXAMPLE $ENV{VKY_EXAMPLE})

# Compilation function .
function(compile_example example_file)
    message("Compile example ${example_file}")
    get_filename_component(example_name ${example_file} NAME_WE)
    add_executable(${example_name} ${example_file})
    target_visky(${example_name})
endfunction()


set(VKY_EXAMPLE_PATH ${CMAKE_SOURCE_DIR}/examples/${VKY_EXAMPLE})
if (${VKY_EXAMPLE}xxx STREQUAL "xxx")
    # Compile all examples
    file(GLOB examples "examples/*.c" "examples/*.cpp")
    foreach(example_file ${examples})
        compile_example(${example_file})
    endforeach()
elseif(EXISTS "${VKY_EXAMPLE_PATH}.c")
    # Or compile a single C example
    compile_example("${VKY_EXAMPLE_PATH}.c")
elseif(EXISTS "${VKY_EXAMPLE_PATH}.cpp")
    # Or compile a single C++ example
    compile_example("${VKY_EXAMPLE_PATH}.cpp")
elseif(NOT ${VKY_EXAMPLE} STREQUAL "test")
    message("WARNING: example '${VKY_EXAMPLE}' not found")
endif()



# -------------------------------------------------------------------------------------------------
# Test build
# -------------------------------------------------------------------------------------------------

set(TEST_CASES all)


message("Compile tests")
add_executable(visky_test test/test.c)
target_visky(visky_test)
function(compile_test test_case)
    set(test_name "test_${test_case}")
    add_test(${test_name} visky_test ${test_case})
endfunction()

foreach(test_case ${TEST_CASES})
    compile_test(${test_case})
endforeach()


# experimental
add_executable(freetype_example experimental/freetype-example.c)
target_include_directories(freetype_example PUBLIC ${INCL_DIRS})
target_link_libraries(freetype_example freetype -lm)
