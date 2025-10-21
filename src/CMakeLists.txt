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
set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS}
    LOG_USE_COLOR
    ENABLE_VALIDATION_LAYERS=${ENABLE_VALIDATION_LAYERS}
    DEBUG=$<IF:$<CONFIG:Debug>,1,0>

    # OS.
    OS_LINUX=${OS_LINUX}
    OS_MACOS=${OS_MACOS}
    OS_WINDOWS=${OS_WINDOWS}

    # Compiler.
    CC_MSVC=${CC_MSVC}
    CC_GCC=${CC_GCC}
    CC_CLANG=${CC_CLANG}
)

if(DVZ_WITH_MIMALLOC)
    list(APPEND COMPILE_DEFINITIONS
        DVZ_HAS_MIMALLOC=$<IF:$<CONFIG:Release>,1,0>)
else()
    list(APPEND COMPILE_DEFINITIONS DVZ_HAS_MIMALLOC=0)
endif()

if(DVZ_USE_MIMALLOC_RELEASE_DEFAULT AND DVZ_WITH_MIMALLOC)
    list(APPEND COMPILE_DEFINITIONS
        DVZ_ALLOCATOR_DEFAULT_MIMALLOC=$<IF:$<CONFIG:Release>,1,0>)
else()
    list(APPEND COMPILE_DEFINITIONS DVZ_ALLOCATOR_DEFAULT_MIMALLOC=0)
endif()

if(DVZ_ENABLE_ASAN_IN_DEBUG)
    list(APPEND COMPILE_DEFINITIONS DVZ_ENABLE_ASAN_IN_DEBUG=1)
else()
    list(APPEND COMPILE_DEFINITIONS DVZ_ENABLE_ASAN_IN_DEBUG=0)
endif()

target_compile_definitions(datoviz PUBLIC ${COMPILE_DEFINITIONS})

# -------------------------------------------------------------------------------------------------
# Components
# -------------------------------------------------------------------------------------------------

# Add source modules
add_subdirectory(common)
add_subdirectory(ds)
add_subdirectory(fileio)
add_subdirectory(math)
add_subdirectory(thread)

# -------------------------------------------------------------------------------------------------
# Include and linking
# -------------------------------------------------------------------------------------------------

# Link object modules
target_link_libraries(datoviz
    PRIVATE
    datoviz_common
    datoviz_ds
    datoviz_fileio
    datoviz_math
    datoviz_thread
)

if(DVZ_WITH_MIMALLOC)
    target_link_libraries(datoviz PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
endif()

# Track core targets for optional coverage instrumentation.
set(_dvz_tracked_targets
    datoviz
    datoviz_common
    datoviz_ds
    datoviz_fileio
    datoviz_math
    datoviz_thread
)

foreach(_dvz_target ${_dvz_tracked_targets})
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

if(DVZ_ENABLE_ASAN_IN_DEBUG)
    set(_dvz_override "${DVZ_SANITIZER}")
    string(STRIP "${_dvz_override}" _dvz_override)
    string(TOLOWER "${_dvz_override}" _dvz_override)

    if(_dvz_override STREQUAL "off" OR _dvz_override STREQUAL "none")
        set(_dvz_override "")
    endif()

    if(_dvz_override STREQUAL "" OR _dvz_override STREQUAL "asan")
        if(CC_MSVC)
            set(DVZ_SANITIZER_FLAGS "/fsanitize=address")
            set(DVZ_ACTIVE_SANITIZERS "address")
        elseif(CC_CLANG)
            if(OS_WINDOWS)
                set(DVZ_SANITIZER_FLAGS "-fsanitize=address")
                set(DVZ_ACTIVE_SANITIZERS "address")
            else()
                set(DVZ_SANITIZER_FLAGS "-fsanitize=address,undefined")
                set(DVZ_ACTIVE_SANITIZERS "address;undefined")

                if(NOT OS_MACOS)
                    set(DVZ_SANITIZER_FLAGS "-fsanitize=address,undefined,leak")
                    set(DVZ_ACTIVE_SANITIZERS "address;undefined;leak")
                endif()
            endif()
        elseif(CC_GCC)
            if(OS_WINDOWS)
                set(DVZ_SANITIZER_FLAGS "")
                set(DVZ_ACTIVE_SANITIZERS "")
            else()
                set(DVZ_SANITIZER_FLAGS "-fsanitize=address")
                set(DVZ_ACTIVE_SANITIZERS "address")

                if(NOT OS_MACOS)
                    set(DVZ_SANITIZER_FLAGS "-fsanitize=address,undefined,leak")
                    set(DVZ_ACTIVE_SANITIZERS "address;undefined;leak")
                endif()
            endif()
        endif()
    elseif(_dvz_override STREQUAL "msan")
        if(CC_CLANG AND NOT OS_MACOS)
            set(DVZ_SANITIZER_FLAGS
                "-fsanitize=memory"
                "-fsanitize-memory-track-origins"
                "-fsanitize-blacklist=${PROJECT_SOURCE_DIR}/sanitizers/msan.ignore")
            set(DVZ_ACTIVE_SANITIZERS "memory")
        endif()
    elseif(_dvz_override STREQUAL "tsan")
        if(CC_CLANG)
            set(DVZ_SANITIZER_FLAGS "-fsanitize=thread")
            set(DVZ_ACTIVE_SANITIZERS "thread")
        endif()
    elseif(_dvz_override)
        set(DVZ_SANITIZER_FLAGS "${DVZ_SANITIZER}")

        if(DVZ_SANITIZER MATCHES "memory")
            set(DVZ_ACTIVE_SANITIZERS "memory")
        elseif(DVZ_SANITIZER MATCHES "thread")
            set(DVZ_ACTIVE_SANITIZERS "thread")
        endif()
    endif()
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
