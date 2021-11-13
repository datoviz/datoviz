# -------------------------------------------------------------------------------------------------
# CMake build script
# -------------------------------------------------------------------------------------------------

set(CMAKE_VERBOSE_MAKEFILE OFF)
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

# TODO: refactor these options
option(DATOVIZ_WITH_PNG "Build Datoviz with PNG support" OFF)
option(DATOVIZ_WITH_GLSLANG "Build Datoviz with glslang support" OFF)
option(DATOVIZ_WITH_SWIFTSHADER "Build Datoviz with Swiftshader support" OFF)
option(DATOVIZ_WITH_CLI "Build Datoviz command-line interface with tests and demos" ON)


# -------------------------------------------------------------------------------------------------
# Build type
# -------------------------------------------------------------------------------------------------

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD)
set(CMAKE_C_STANDARD_REQUIRED True)


# -------------------------------------------------------------------------------------------------
# Build dependencies
# -------------------------------------------------------------------------------------------------

# ---- Vulkan -------------------------------------------------------------------------------------
find_package(Vulkan)
if (Vulkan_FOUND)
message(STATUS "Found Vulkan")
else()
message(FATAL_ERROR "-- Could NOT find Vulkan")
endif()

# TODO: PNG
# find_package(PNG REQUIRED)
# include_directories(${PNG_INCLUDE_DIR})
# target_link_libraries(${MY_EXEC} ${PNG_LIBRARY})

# ---- cglm ---------------------------------------------------------------------------------------
FetchContent_Declare(
    cglm
    GIT_REPOSITORY  https://github.com/recp/cglm/
    GIT_TAG         v0.8.3
    )
    FetchContent_MakeAvailable(cglm)

# ---- glfw ---------------------------------------------------------------------------------------
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY  https://github.com/glfw/glfw/
    GIT_TAG         3.3.5
    )
    FetchContent_MakeAvailable(glfw)

# ---- glslc --------------------------------------------------------------------------------------
if (DATOVIZ_WITH_GLSLANG)
    FetchContent_Declare(
    glslang
    GIT_REPOSITORY https://github.com/KhronosGroup/glslang
    GIT_TAG        master-tot
    )
    FetchContent_MakeAvailable(glslang)
endif()


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
# Compile definitions
# -------------------------------------------------------------------------------------------------

set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS}
    LOG_USE_COLOR
    ENABLE_VALIDATION_LAYERS=1
    ROOT_DIR=\"${CMAKE_SOURCE_DIR}\"

    OS_LINUX=${OS_LINUX}
    OS_MACOS=${OS_MACOS}
    OS_WIN32=${OS_WIN32}

    CC_MSVC=${CC_MSVC}
    CC_GCC=${CC_GCC}
    CC_CLANG=${CC_CLANG}
)


# -------------------------------------------------------------------------------------------------
# Compiler options
# -------------------------------------------------------------------------------------------------

# ---- common compiler options --------------------------------------------------------------------
# NOTE: need to remove -pg (gprof profiling) in RELEASE mode?
set(COMMON_FLAGS "-g -m64 -pedantic -Wall -Wextra -Werror=vla \
    -Wcast-align -Wcast-qual -Wredundant-decls -Wswitch-default -Wdisabled-optimization \
    -Wformat=2 -Winit-self -Wno-missing-field-initializers -Wno-unused-result \
    -Wmissing-declarations -Wmissing-include-dirs -Wno-format-nonliteral \
    -Wshadow -Wsign-conversion -Wstrict-overflow=5 -Wundef -Wno-unused-parameter \
    -Wno-unused-function -Wno-variadic-macros")

# ---- compiler-specific options ------------------------------------------------------------------
if(CC_GCC)
    # gcc
    set(COMMON_FLAGS "${COMMON_FLAGS} -Wlogical-op -fdiagnostics-color=always")
    if (NOT OS_WIN32)
        set(COMMON_FLAGS "${COMMON_FLAGS} -pg")
    endif()

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -std=gnu11 -Wmissing-prototypes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS} -std=c++17")

elseif(CC_CLANG)
    # clang
    set(COMMON_FLAGS "${COMMON_FLAGS} -Wno-unused-parameter -fcolor-diagnostics -Wno-missing-braces -Wmissing-prototypes")

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS} -std=c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")
    set(CMAKE_OBJC_FLAGS
        "${CMAKE_OBJC_FLAGS} ${COMMON_FLAGS}
        -Wno-partial-availability -Wno-unguarded-availability-new")

elseif(CC_MSVC)
    # MSVC

    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

endif()


# -------------------------------------------------------------------------------------------------
# Include directories
# -------------------------------------------------------------------------------------------------

set(INCL_DIRS
    ${CMAKE_SOURCE_DIR}/include/datoviz

    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui/backends
)

# ---- OS-specific directories --------------------------------------------------------------------
if(${OS_MACOS})
    set(INCL_DIRS ${INCL_DIRS} /usr/local/include)
elseif(${OS_WIN32})
    set(INCL_DIRS ${INCL_DIRS} $ENV{VULKAN_SDK}/include $ENV{CGLM_INCLUDE})
    find_path(MINGW_DIR NAMES "gcc.exe")

endif()


# -------------------------------------------------------------------------------------------------
# Linking libraries
# -------------------------------------------------------------------------------------------------

set(LINK_LIBS glfw cglm) # Vulkan::Vulkan
if(NOT ${WIN32})
    set(LINK_LIBS m dl pthread ${LINK_LIBS})
endif()

# ---- OS-specific linking libraries --------------------------------------------------------------
if(${OS_MACOS})
    link_directories(/usr/local/lib)
elseif(${OS_WIN32})
    link_directories($ENV{VULKAN_SDK}\\Lib $ENV{VULKAN_SDK}\\Bin) # $ENV{CGLM_LIB} ${MINGW_DIR})
    set(LINK_LIBS ${LINK_LIBS})
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
# Source files
# -------------------------------------------------------------------------------------------------

set(sources
    "src/_atomic.cpp"
    "src/log.c"
    "src/fileio.c"

    "external/tinycthread.c"
)


set(cpp_sources
    ${sources}
)


# -------------------------------------------------------------------------------------------------
# Datoviz shared library
# -------------------------------------------------------------------------------------------------

add_library(libdatoviz SHARED ${sources})
set_source_files_properties(${cpp_sources} PROPERTIES COMPILE_FLAGS -w)
set_target_properties(libdatoviz PROPERTIES
    VERSION ${DATOVIZ_VERSION}
    SOVERSION 1
    # PUBLIC_HEADER include/datoviz/datoviz.h
    )
target_compile_definitions(libdatoviz PUBLIC ${COMPILE_DEFINITIONS})
target_include_directories(libdatoviz PUBLIC ${INCL_DIRS})
target_link_libraries(libdatoviz ${LINK_LIBS})


# -------------------------------------------------------------------------------------------------
# CLI tool
# -------------------------------------------------------------------------------------------------

if (DATOVIZ_WITH_CLI)
    set(cli
        "cli/main.c"
        "external/tinycthread.c"  # NOTE: for mutex functions not exported in libdatoviz
        "tests/test_thread.c"
        "tests/test_obj.c"
        "tests/test.c"
    )

    add_executable(datovizcli ${cli})
    set_target_properties(datovizcli PROPERTIES OUTPUT_NAME "datoviz")
    target_compile_definitions(datovizcli PUBLIC ${COMPILE_DEFINITIONS})
    target_include_directories(datovizcli PUBLIC ${INCL_DIRS} ${CMAKE_SOURCE_DIR}/tests)
    target_link_libraries(datovizcli libdatoviz)
endif()
