set(CMAKE_VERBOSE_MAKEFILE OFF)

cmake_minimum_required(VERSION 3.10)
enable_testing()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include("cmake/colormsg.cmake")
include(FetchContent)

set(CMAKE_C_STANDARD)
set(CMAKE_C_STANDARD_REQUIRED True)


# Options
option(DATOVIZ_WITH_PNG "Build Datoviz with PNG support" OFF)
option(DATOVIZ_WITH_GLSLANG "Build Datoviz with glslang support" OFF)
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
set(DATOVIZ_VERSION 0.0.1)
project(datoviz VERSION ${DATOVIZ_VERSION} DESCRIPTION "datoviz")

# DEBUG/RELEASE
set(DEBUG 1)
set(CMAKE_BUILD_TYPE Debug)


# -------------------------------------------------------------------------------------------------
# Dependencies
# -------------------------------------------------------------------------------------------------

# Vulkan
find_package(Vulkan)
if (Vulkan_FOUND)
    message(STATUS "Found Vulkan")
else()
    message(FATAL_ERROR "-- Could NOT find Vulkan")
endif()

# cglm
FetchContent_Declare(
    cglm
    GIT_REPOSITORY  https://github.com/recp/cglm/
    GIT_TAG         v0.8.3
)
FetchContent_MakeAvailable(cglm)

# glfw3
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY  https://github.com/glfw/glfw/
    GIT_TAG         3.3.4
)
FetchContent_MakeAvailable(glfw)

# glslc
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


# -------------------------------------------------------------------------------------------------
# Include and linking directories
# -------------------------------------------------------------------------------------------------

set(INCL_DIRS
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/imgui/backends
    ${CMAKE_SOURCE_DIR}/include/datoviz
    # ${CMAKE_SOURCE_DIR}/tests
)
set(LINK_LIBS m glfw cglm pthread) # Vulkan::Vulkan
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



# -------------------------------------------------------------------------------------------------
# Build variables and macros
# -------------------------------------------------------------------------------------------------

# Pass definitions
set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS}
    LOG_USE_COLOR
    ENABLE_VALIDATION_LAYERS=1
    ROOT_DIR=\"${CMAKE_SOURCE_DIR}\"

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
# Datoviz library build
# -------------------------------------------------------------------------------------------------

# Define the datoviz library target
file(GLOB sources
    "src/log.c"
    "src/fileio.c"
    "cli/main.c"

    # "external/*.c*"
    # "external/imgui/*.c*"
    # "external/imgui/backends/imgui_impl_glfw.*"
    # "external/imgui/backends/imgui_impl_vulkan.*"
)

file(GLOB cpp_sources
    "external/*.c*"
    # "external/imgui/*.h*" "external/imgui/*.c*"
    # "external/imgui/backends/imgui_impl_glfw.*"
    # "external/imgui/backends/imgui_impl_vulkan.*"
)

set_source_files_properties(${cpp_sources} PROPERTIES COMPILE_FLAGS -w)
add_library(datoviz SHARED ${sources})

set_target_properties(datoviz PROPERTIES
    VERSION ${DATOVIZ_VERSION}
    SOVERSION 1
    # PUBLIC_HEADER include/datoviz/datoviz.h
    )

target_compile_definitions(datoviz PUBLIC ${COMPILE_DEFINITIONS})
target_include_directories(datoviz PUBLIC ${INCL_DIRS})
target_link_libraries(datoviz ${LINK_LIBS})
if(${WIN32})
    target_link_libraries(datoviz -static gcc stdc++ winpthread -dynamic)
endif()



# -------------------------------------------------------------------------------------------------
# CLI tool build
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


if (DATOVIZ_WITH_CLI)
    file(GLOB cli
        "cli/main.c"
        # "tests/test_*.c"
    )

    add_executable(datovizcli ${cli})
    target_datoviz(datovizcli)
    set_target_properties(datovizcli PROPERTIES OUTPUT_NAME "datoviz")
    # add_test(NAME datoviz_test COMMAND datovizcli test)
endif()
