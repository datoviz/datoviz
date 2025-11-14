# -------------------------------------------------------------------------------------------------
# Meta CMake file
# -------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)
project(datoviz VERSION 0.4 LANGUAGES C CXX)

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

if(DVZ_ENABLE_GPROF)
    add_compile_options(
        $<$<COMPILE_LANGUAGE:C>:-pg>
        $<$<COMPILE_LANGUAGE:CXX>:-pg>
    )
    add_link_options(-pg)
endif()

# Track whether CUDA + NVENC support is available so subdirectories can make decisions.
set(DVZ_HAS_CUDA 0)
set(DVZ_NVENC_LIB "")
set(DVZ_NVENC_LIB_DIR "")
option(DVZ_ENABLE_KVAZAAR "Enable Kvazaar software HEVC backend" ON)
option(DVZ_WITH_GLFW "Enable the GLFW window backend" OFF)
set(DVZ_HAS_KVZ 0)

set(DVZ_GLFW_DIR "${PROJECT_SOURCE_DIR}/external/glfw")
if(DVZ_WITH_GLFW)
    if(EXISTS "${DVZ_GLFW_DIR}/CMakeLists.txt")
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

        add_subdirectory(
            "${DVZ_GLFW_DIR}"
            "${CMAKE_CURRENT_BINARY_DIR}/external/glfw"
            EXCLUDE_FROM_ALL)

        if(TARGET glfw)
            add_library(glfw::glfw ALIAS glfw)
        endif()
    else()
        message(WARNING "GLFW submodule missing — disable DVZ_WITH_GLFW or run 'git submodule update --init external/glfw'")
    endif()
endif()

if(DVZ_ENABLE_CUDA)
    find_package(CUDAToolkit QUIET)

    if(NOT CUDAToolkit_FOUND)
        message(WARNING "CUDA toolkit not found — disabling DVZ_ENABLE_CUDA")
        set(DVZ_ENABLE_CUDA OFF CACHE BOOL "Enable CUDA interop hooks and tests" FORCE)
    else()
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
            set(DVZ_HAS_CUDA 1)
            set(DVZ_NVENC_LIB "${_dvz_nvenc_lib}" CACHE FILEPATH "NVENC library resolved by CMake" FORCE)
            set(DVZ_NVENC_LIB_DIR "${_dvz_nvenc_lib_dir}" CACHE PATH "Directory searched for NVENC" FORCE)
        else()
            message(WARNING "CUDA toolkit found but NVENC library missing — disabling DVZ_ENABLE_CUDA")
            set(DVZ_ENABLE_CUDA OFF CACHE BOOL "Enable CUDA interop hooks and tests" FORCE)
        endif()
    endif()
endif()

if(NOT DVZ_HAS_CUDA)
    unset(DVZ_NVENC_LIB CACHE)
    unset(DVZ_NVENC_LIB_DIR CACHE)
endif()

set(DVZ_HAS_CUDA "${DVZ_HAS_CUDA}" CACHE INTERNAL "CUDA availability flag" FORCE)

if(DVZ_ENABLE_KVAZAAR)
    set(_dvz_kvz_dir "${PROJECT_SOURCE_DIR}/external/kvazaar")
    if(EXISTS "${_dvz_kvz_dir}/CMakeLists.txt")
        if(DEFINED BUILD_SHARED_LIBS)
            set(_dvz_prev_build_shared_libs "${BUILD_SHARED_LIBS}")
            set(_dvz_prev_build_shared_libs_defined 1)
        else()
            set(_dvz_prev_build_shared_libs "")
            set(_dvz_prev_build_shared_libs_defined 0)
        endif()
        if(DEFINED BUILD_TESTS)
            set(_dvz_prev_build_tests "${BUILD_TESTS}")
            set(_dvz_prev_build_tests_defined 1)
        else()
            set(_dvz_prev_build_tests "")
            set(_dvz_prev_build_tests_defined 0)
        endif()

        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(BUILD_KVAZAAR_BINARY OFF CACHE BOOL "" FORCE)
        set(USE_CRYPTO OFF CACHE BOOL "" FORCE)

        add_subdirectory(
            "${_dvz_kvz_dir}"
            "${CMAKE_CURRENT_BINARY_DIR}/external/kvazaar"
            EXCLUDE_FROM_ALL)

        if(TARGET kvazaar)
            add_library(kvazaar::kvazaar ALIAS kvazaar)
            set_target_properties(kvazaar PROPERTIES POSITION_INDEPENDENT_CODE ON)
            set(DVZ_HAS_KVZ 1)
        endif()

        if(_dvz_prev_build_shared_libs_defined)
            set(BUILD_SHARED_LIBS "${_dvz_prev_build_shared_libs}" CACHE BOOL "" FORCE)
        else()
            unset(BUILD_SHARED_LIBS CACHE)
        endif()
        if(_dvz_prev_build_tests_defined)
            set(BUILD_TESTS "${_dvz_prev_build_tests}" CACHE BOOL "" FORCE)
        else()
            unset(BUILD_TESTS CACHE)
        endif()
        unset(BUILD_KVAZAAR_BINARY CACHE)
        unset(USE_CRYPTO CACHE)

        unset(_dvz_prev_build_shared_libs)
        unset(_dvz_prev_build_shared_libs_defined)
        unset(_dvz_prev_build_tests)
        unset(_dvz_prev_build_tests_defined)
    else()
        message(WARNING "Kvazaar submodule missing — disable DVZ_ENABLE_KVAZAAR or run 'git submodule update --init external/kvazaar'")
    endif()
endif()

set(DVZ_HAS_KVZ "${DVZ_HAS_KVZ}" CACHE INTERNAL "Kvazaar availability flag" FORCE)

# Third-party dependencies
set(DVZ_WITH_MIMALLOC OFF)
set(DVZ_EXTERNAL_DIR "${PROJECT_SOURCE_DIR}/external")
set(DVZ_MIMALLOC_DIR "${DVZ_EXTERNAL_DIR}/mimalloc")

if(EXISTS "${DVZ_MIMALLOC_DIR}/CMakeLists.txt")
    set(DVZ_WITH_MIMALLOC ON)
    set(MI_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(MI_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(MI_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(MI_INSTALL_TOPLEVEL OFF CACHE BOOL "" FORCE)
    add_subdirectory("${DVZ_MIMALLOC_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/external/mimalloc" EXCLUDE_FROM_ALL)
endif()

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

# Add subdirectories
add_subdirectory(src)
add_subdirectory(testing)

get_property(_dvz_targets GLOBAL PROPERTY DVZ_ALL_TARGETS)
list(REMOVE_DUPLICATES _dvz_targets)
get_property(_dvz_compile_definitions GLOBAL PROPERTY DVZ_COMPILE_DEFINITIONS)

foreach(_dvz_target ${_dvz_targets})
    if(TARGET ${_dvz_target})
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
message(STATUS "DVZ_WITH_MIMALLOC (detected): ${DVZ_WITH_MIMALLOC}")

if(DEFINED DVZ_USE_VALIDATION)
    message(STATUS "DVZ_USE_VALIDATION: ${DVZ_USE_VALIDATION} (Release configs disable validation unless explicitly ON)")
endif()

message(STATUS "-------------------------------------------------------------")
message(STATUS "")

# Install headers and library
install(DIRECTORY include/ DESTINATION include)
