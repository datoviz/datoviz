# -------------------------------------------------------------------------------------------------
# Source CMake file
# -------------------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------------------
# Detect the OS
# -------------------------------------------------------------------------------------------------
#
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
#
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
# Compiler options
# -------------------------------------------------------------------------------------------------
set(DVZ_COMMON_COMPILE_OPTIONS "")
set(DVZ_COMPILE_OPTIONS_C "")
set(DVZ_COMPILE_OPTIONS_CXX "")
set(DVZ_COMPILE_OPTIONS_OBJC "")

# Default: OpenMP remains disabled until detection is reintroduced.
set(HAS_OPENMP 0)
set(DVZ_FPNG_NO_SSE 1)

if(CC_GCC OR CC_CLANG)
    list(APPEND DVZ_COMMON_COMPILE_OPTIONS
        -g
        -m64
        -pedantic
        -Wall
        -Wextra
        -Werror=vla
        -Wcast-align
        -Wcast-qual
        -Wredundant-decls
        -Wswitch-default
        -Wmissing-declarations
        -Wmissing-include-dirs
        -Wno-unused-result
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-parameter
        -Wno-strict-overflow
        -Wsign-conversion
        -Wshadow
        -Wformat=2
        -Wno-format-nonliteral
        -Winit-self
        -Wundef
        $<$<CONFIG:Debug>:-O0>
        $<$<NOT:$<CONFIG:Debug>>:-O2>
    )
endif()

if(CC_GCC)
    list(APPEND DVZ_COMMON_COMPILE_OPTIONS
        -Wlogical-op
        -Wno-stringop-overread
        -fvisibility=hidden
        -fdiagnostics-color=always
        -Wno-unused-but-set-variable
    )

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64|i.86")
        list(APPEND DVZ_COMMON_COMPILE_OPTIONS
            -msse4.1
            -mpclmul
        )
        set(DVZ_FPNG_NO_SSE 0)
    else()
        set(DVZ_FPNG_NO_SSE 1)
    endif()

    if(HAS_OPENMP AND OpenMP_CXX_FLAGS)
        separate_arguments(_dvz_openmp_flags NATIVE_COMMAND "${OpenMP_CXX_FLAGS}")
        list(APPEND DVZ_COMMON_COMPILE_OPTIONS ${_dvz_openmp_flags})
    endif()

    list(APPEND DVZ_COMPILE_OPTIONS_C
        -std=gnu11
        -Wmissing-prototypes
        -Wimplicit-function-declaration
        -Wstrict-prototypes
    )

    list(APPEND DVZ_COMPILE_OPTIONS_CXX
        -std=c++17
    )
elseif(CC_CLANG)
    list(APPEND DVZ_COMMON_COMPILE_OPTIONS
        -fcolor-diagnostics
        -Wno-missing-braces
        -fdiagnostics-color=always
    )

    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        list(APPEND DVZ_COMMON_COMPILE_OPTIONS
            -msse4.1
            -mpclmul
        )
        set(DVZ_FPNG_NO_SSE 0)
    else()
        set(DVZ_FPNG_NO_SSE 1)
    endif()

    list(APPEND DVZ_COMPILE_OPTIONS_C
        -std=gnu11
        -Wmissing-prototypes
    )

    if(APPLE)
        list(APPEND DVZ_COMPILE_OPTIONS_CXX
            -stdlib=libc++
        )
    endif()

    list(APPEND DVZ_COMPILE_OPTIONS_CXX -std=c++17)

    set(DVZ_COMPILE_OPTIONS_OBJC
        -Wno-partial-availability
        -Wno-unguarded-availability-new
    )
elseif(CC_MSVC)
    list(APPEND DVZ_COMPILE_OPTIONS_C /std:c11)
    list(APPEND DVZ_COMPILE_OPTIONS_CXX /std:c++17)
endif()

set(DVZ_COMMON_COMPILE_OPTIONS "${DVZ_COMMON_COMPILE_OPTIONS}" CACHE INTERNAL "Datoviz shared compiler options" FORCE)
set(DVZ_COMPILE_OPTIONS_C "${DVZ_COMPILE_OPTIONS_C}" CACHE INTERNAL "Datoviz C compiler options" FORCE)
set(DVZ_COMPILE_OPTIONS_CXX "${DVZ_COMPILE_OPTIONS_CXX}" CACHE INTERNAL "Datoviz C++ compiler options" FORCE)
set(DVZ_COMPILE_OPTIONS_OBJC "${DVZ_COMPILE_OPTIONS_OBJC}" CACHE INTERNAL "Datoviz Objective-C compiler options" FORCE)
set(HAS_OPENMP "${HAS_OPENMP}" CACHE INTERNAL "OpenMP availability flag" FORCE)

# -------------------------------------------------------------------------------------------------
# Create the library
# -------------------------------------------------------------------------------------------------

# Create the shared library combining all object libraries
add_library(datoviz SHARED
    "empty.c"
)

# Output name
set_target_properties(datoviz PROPERTIES OUTPUT_NAME "datoviz")

# Install rule
install(TARGETS datoviz
    LIBRARY DESTINATION lib
)

# -------------------------------------------------------------------------------------------------
# Compile definitions
# -------------------------------------------------------------------------------------------------
#
set(HAS_CUDA 0)

if(DVZ_ENABLE_CUDA)
    set(HAS_CUDA 1)
endif()

set(DVZ_COMPILE_DEFINITIONS ${DVZ_COMPILE_DEFINITIONS}
    LOG_USE_COLOR
    ENABLE_VALIDATION_LAYERS=${ENABLE_VALIDATION_LAYERS}
    DEBUG=$<IF:$<CONFIG:Debug>,1,0>

    # Required so volk supplies the Vulkan entry points instead of the SDK prototypes.
    VK_NO_PROTOTYPES

    # OS.
    OS_LINUX=${OS_LINUX}
    OS_MACOS=${OS_MACOS}
    OS_WINDOWS=${OS_WINDOWS}

    # Compiler.
    CC_MSVC=${CC_MSVC}
    CC_GCC=${CC_GCC}
    CC_CLANG=${CC_CLANG}

    # TODO: ZLIB
    HAS_ZLIB=0
    HAS_OPENMP=${HAS_OPENMP}
    HAS_CUDA=${HAS_CUDA}
)

if(DVZ_WITH_MIMALLOC)
    list(APPEND DVZ_COMPILE_DEFINITIONS
        DVZ_HAS_MIMALLOC=$<IF:$<CONFIG:Release>,1,0>)
else()
    list(APPEND DVZ_COMPILE_DEFINITIONS DVZ_HAS_MIMALLOC=0)
endif()

if(DVZ_USE_MIMALLOC_RELEASE_DEFAULT AND DVZ_WITH_MIMALLOC)
    list(APPEND DVZ_COMPILE_DEFINITIONS
        DVZ_ALLOCATOR_DEFAULT_MIMALLOC=$<IF:$<CONFIG:Release>,1,0>)
else()
    list(APPEND DVZ_COMPILE_DEFINITIONS DVZ_ALLOCATOR_DEFAULT_MIMALLOC=0)
endif()

if(DVZ_ENABLE_ASAN_IN_DEBUG)
    list(APPEND DVZ_COMPILE_DEFINITIONS DVZ_ENABLE_ASAN_IN_DEBUG=1)
else()
    list(APPEND DVZ_COMPILE_DEFINITIONS DVZ_ENABLE_ASAN_IN_DEBUG=0)
endif()

list(APPEND DVZ_COMPILE_DEFINITIONS FPNG_NO_SSE=${DVZ_FPNG_NO_SSE})

target_compile_definitions(datoviz PUBLIC ${DVZ_COMPILE_DEFINITIONS})

set(DVZ_COMPILE_DEFINITIONS "${DVZ_COMPILE_DEFINITIONS}" PARENT_SCOPE)
set_property(GLOBAL PROPERTY DVZ_COMPILE_DEFINITIONS "${DVZ_COMPILE_DEFINITIONS}")

# -------------------------------------------------------------------------------------------------
# External dependencies
# -------------------------------------------------------------------------------------------------
#
add_subdirectory(${PROJECT_SOURCE_DIR}/external/volk ${CMAKE_CURRENT_BINARY_DIR}/external/volk EXCLUDE_FROM_ALL)

# -------------------------------------------------------------------------------------------------
# Components
# -------------------------------------------------------------------------------------------------
#
add_subdirectory(common)
add_subdirectory(ds)
add_subdirectory(fileio)
add_subdirectory(math)
add_subdirectory(thread)
add_subdirectory(vk)

# -------------------------------------------------------------------------------------------------
# Include and linking
# -------------------------------------------------------------------------------------------------
#
set(DVZ_COMPONENTS
    datoviz_common
    datoviz_ds
    datoviz_fileio
    datoviz_math
    datoviz_thread
    datoviz_vk
)

# Link object modules
target_link_libraries(datoviz
    PRIVATE
    ${DVZ_COMPONENTS}
    datoviz_volk
)

if(DVZ_THREADING_TARGET)
    target_link_libraries(datoviz PRIVATE ${DVZ_THREADING_TARGET})
endif()

if(DVZ_WITH_MIMALLOC)
    target_link_libraries(datoviz PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
endif()

foreach(_dvz_target datoviz ${DVZ_COMPONENTS})
    if(TARGET ${_dvz_target})
        set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS ${_dvz_target})
    endif()
endforeach()

# Public headers
target_include_directories(datoviz
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

# -------------------------------------------------------------------------------------------------
# Sanitizers
# -------------------------------------------------------------------------------------------------
#
set(DVZ_SANITIZER_FLAGS "")
set(DVZ_ACTIVE_SANITIZERS "")

set(_dvz_sanitizer "${DVZ_SANITIZER}")
string(STRIP "${_dvz_sanitizer}" _dvz_sanitizer)
set(_dvz_sanitizer_user 0)

if(NOT _dvz_sanitizer STREQUAL "")
    set(_dvz_sanitizer_user 1)
endif()

string(TOLOWER "${_dvz_sanitizer}" _dvz_sanitizer)

if(_dvz_sanitizer STREQUAL "off" OR _dvz_sanitizer STREQUAL "none")
    # Sanitizers explicitly disabled via DVZ_SANITIZER override.
    set(_dvz_sanitizer "")
endif()

if(NOT _dvz_sanitizer_user AND DVZ_ENABLE_ASAN_IN_DEBUG)
    set(_dvz_sanitizer "asan")
endif()

if(_dvz_sanitizer STREQUAL "asan")
    set(_dvz_asan_ignore "")

    if(EXISTS "${PROJECT_SOURCE_DIR}/sanitizers/asan.ignore")
        set(_dvz_asan_ignore "${PROJECT_SOURCE_DIR}/sanitizers/asan.ignore")
    endif()

    if(CC_MSVC)
        list(APPEND DVZ_SANITIZER_FLAGS "/fsanitize=address")
        list(APPEND DVZ_ACTIVE_SANITIZERS "address")
    elseif(CC_CLANG)
        if(OS_WINDOWS)
            list(APPEND DVZ_SANITIZER_FLAGS "-fsanitize=address")
            list(APPEND DVZ_ACTIVE_SANITIZERS "address")
        else()
            list(APPEND DVZ_SANITIZER_FLAGS "-fsanitize=address")
            list(APPEND DVZ_SANITIZER_FLAGS "-fsanitize=undefined")
            list(APPEND DVZ_SANITIZER_FLAGS "-fno-omit-frame-pointer")
            list(APPEND DVZ_SANITIZER_FLAGS "-fno-optimize-sibling-calls")
            list(APPEND DVZ_SANITIZER_FLAGS "-O1")

            if(_dvz_asan_ignore)
                list(APPEND DVZ_SANITIZER_FLAGS "-fsanitize-ignorelist=${_dvz_asan_ignore}")
            endif()

            list(APPEND DVZ_ACTIVE_SANITIZERS "address" "undefined")

            if(NOT OS_MACOS)
                list(APPEND DVZ_SANITIZER_FLAGS "-fsanitize=leak")
                list(APPEND DVZ_ACTIVE_SANITIZERS "leak")
            endif()
        endif()
    elseif(CC_GCC)
        if(OS_WINDOWS)
        # GCC ASan is not available on Windows Mingw builds by default.
        else()
            list(APPEND DVZ_SANITIZER_FLAGS "-fsanitize=address")
            list(APPEND DVZ_ACTIVE_SANITIZERS "address")

            if(NOT OS_MACOS)
                list(APPEND DVZ_SANITIZER_FLAGS "-fsanitize=undefined")
                list(APPEND DVZ_SANITIZER_FLAGS "-fsanitize=leak")
                list(APPEND DVZ_ACTIVE_SANITIZERS "undefined" "leak")
            endif()

            if(_dvz_asan_ignore)
                include(CheckCCompilerFlag)
                set(_dvz_has_ignorelist OFF)
                check_c_compiler_flag("-fsanitize-ignorelist=${_dvz_asan_ignore}" _dvz_has_ignorelist)

                if(_dvz_has_ignorelist)
                    list(APPEND DVZ_SANITIZER_FLAGS "-fsanitize-ignorelist=${_dvz_asan_ignore}")
                endif()

                # GCC historically used -fsanitize-blacklist; fallback removed in v0.4-dev in favour of ignorelist.
            endif()
        endif()
    endif()
elseif(_dvz_sanitizer STREQUAL "msan")
    if(CC_CLANG AND NOT OS_MACOS)
        list(APPEND DVZ_SANITIZER_FLAGS
            "-fsanitize-recover=all"
            "-fsanitize=memory"
            "-fsanitize-memory-track-origins"
            "-fsanitize-ignorelist=${PROJECT_SOURCE_DIR}/sanitizers/msan.ignore")
        list(APPEND DVZ_ACTIVE_SANITIZERS "memory")
    endif()
elseif(_dvz_sanitizer STREQUAL "tsan")
    if(CC_CLANG)
        list(APPEND DVZ_SANITIZER_FLAGS "-fsanitize=thread")
        list(APPEND DVZ_ACTIVE_SANITIZERS "thread")
    endif()
elseif(NOT _dvz_sanitizer STREQUAL "")
    list(APPEND DVZ_SANITIZER_FLAGS "${DVZ_SANITIZER}")

    if(DVZ_SANITIZER MATCHES "memory")
        list(APPEND DVZ_ACTIVE_SANITIZERS "memory")
    elseif(DVZ_SANITIZER MATCHES "thread")
        list(APPEND DVZ_ACTIVE_SANITIZERS "thread")
    endif()
endif()

if(DVZ_ACTIVE_SANITIZERS)
    list(REMOVE_DUPLICATES DVZ_ACTIVE_SANITIZERS)
endif()

if(DVZ_SANITIZER_FLAGS)
    message(STATUS "Debug sanitizers: ${DVZ_SANITIZER_FLAGS}")

    foreach(target_name datoviz datoviz_common datoviz_ds datoviz_fileio datoviz_math datoviz_thread)
        if(TARGET ${target_name})
            target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug>:${DVZ_SANITIZER_FLAGS}>)
            target_link_options(${target_name} PRIVATE $<$<CONFIG:Debug>:${DVZ_SANITIZER_FLAGS}>)
        endif()
    endforeach()
endif()

set(DVZ_SANITIZER_FLAGS "${DVZ_SANITIZER_FLAGS}" CACHE INTERNAL "Sanitizer flags for Debug targets" FORCE)
set(DVZ_ACTIVE_SANITIZERS "${DVZ_ACTIVE_SANITIZERS}" CACHE INTERNAL "Enabled sanitizer names" FORCE)
