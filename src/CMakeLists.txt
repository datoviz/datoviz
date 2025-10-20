# -------------------------------------------------------------------------------------------------
# Source CMake file
# -------------------------------------------------------------------------------------------------

# -------------------------------------------------------------------------------------------------
# Detect the OS
# -------------------------------------------------------------------------------------------------

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

# Public headers
target_include_directories(datoviz
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

if(DVZ_ENABLE_ASAN_IN_DEBUG)
    if(CC_MSVC)
        set(DVZ_ASAN_FLAG "/fsanitize=address")
    else()
        set(DVZ_ASAN_FLAG "-fsanitize=address")
    endif()
    foreach(target_name datoviz datoviz_common datoviz_ds datoviz_fileio datoviz_math datoviz_thread)
        if(TARGET ${target_name})
            target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug>:${DVZ_ASAN_FLAG}>)
            target_link_options(${target_name} PRIVATE $<$<CONFIG:Debug>:${DVZ_ASAN_FLAG}>)
        endif()
    endforeach()
    # Share sanitizer flag with sibling directories.
    set(DVZ_ASAN_FLAG ${DVZ_ASAN_FLAG} PARENT_SCOPE)
endif()
