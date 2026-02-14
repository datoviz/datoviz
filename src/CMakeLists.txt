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
set(OS_UNIX 0)

if(UNIX AND NOT APPLE)
    set(OS_LINUX 1)
    set(OS_UNIX 1)
elseif(APPLE)
    set(OS_MACOS 1)
    set(OS_UNIX 1)
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
set(DVZ_HAS_OPENMP 0)
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

    if(DVZ_HAS_OPENMP AND OpenMP_CXX_FLAGS)
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
set(DVZ_HAS_OPENMP "${DVZ_HAS_OPENMP}" CACHE INTERNAL "OpenMP availability flag" FORCE)

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

if(APPLE)
    # Ensure the shared library can resolve sibling dylibs at runtime via dlopen().
    set_target_properties(datoviz PROPERTIES
        BUILD_RPATH "@loader_path"
        INSTALL_RPATH "@loader_path"
    )
endif()

# -------------------------------------------------------------------------------------------------
# Compile definitions
# -------------------------------------------------------------------------------------------------
#
set(DVZ_HAS_ZLIB 0)

# Validation layers are enabled by default except for Release builds unless manually overridden.
option(DVZ_USE_VALIDATION "Enable Vulkan validation layers and debug utils" ON)

set(ENABLE_VALIDATION_LAYERS
    $<IF:$<OR:$<CONFIG:Release>,$<NOT:$<BOOL:${DVZ_USE_VALIDATION}>>>,0,1>)

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
    OS_UNIX=${OS_UNIX}

    # Compiler.
    CC_MSVC=${CC_MSVC}
    CC_GCC=${CC_GCC}
    CC_CLANG=${CC_CLANG}

    # Feature availability.
    DVZ_HAS_ZLIB=${DVZ_HAS_ZLIB}
    DVZ_HAS_OPENMP=${DVZ_HAS_OPENMP}
    DVZ_HAS_CUDA=${DVZ_HAS_CUDA}
    DVZ_HAS_GLFW=${DVZ_HAS_GLFW}
    DVZ_HAS_KVZ=${DVZ_HAS_KVZ}

    # Vulkan Memory Allocator uses Volk-loaded entry points.
    VMA_STATIC_VULKAN_FUNCTIONS=0
    VMA_DYNAMIC_VULKAN_FUNCTIONS=0
)

if(OS_WINDOWS)
    # Enable Win32 platform surface/interop declarations in Vulkan/Volk headers.
    list(APPEND DVZ_COMPILE_DEFINITIONS VK_USE_PLATFORM_WIN32_KHR)
endif()

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
set(DVZ_CORE_COMPONENTS
    datoviz_common
    datoviz_ds
    datoviz_fileio
    datoviz_math
    datoviz_thread
)

set(DVZ_VK_COMPONENTS
    datoviz_vk
    datoviz_vklite
)

set(DVZ_CANVAS_COMPONENTS
    datoviz_input
    datoviz_window
    datoviz_canvas
    datoviz_stream
    datoviz_video
)

add_subdirectory(common)
add_subdirectory(ds)
add_subdirectory(fileio)
add_subdirectory(math)
add_subdirectory(thread)
add_subdirectory(input)
add_subdirectory(window)
add_subdirectory(canvas)
add_subdirectory(stream)
add_subdirectory(video)
add_subdirectory(vk)
add_subdirectory(vklite)

# -------------------------------------------------------------------------------------------------
# Layered targets
# -------------------------------------------------------------------------------------------------
#
set(DVZ_COMPONENTS
    ${DVZ_CORE_COMPONENTS}
    ${DVZ_VK_COMPONENTS}
    ${DVZ_CANVAS_COMPONENTS}
)

add_library(datoviz_core SHARED "empty.c")
set_target_properties(datoviz_core PROPERTIES OUTPUT_NAME "datoviz_core")

target_link_libraries(datoviz_core
    PRIVATE
    ${DVZ_CORE_COMPONENTS}
)

target_compile_definitions(datoviz_core PUBLIC ${DVZ_COMPILE_DEFINITIONS})

target_include_directories(datoviz_core
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

add_library(datoviz_vk_layer SHARED "empty.c")
set_target_properties(datoviz_vk_layer PROPERTIES OUTPUT_NAME "datoviz_vk")

target_link_libraries(datoviz_vk_layer
    PRIVATE
    datoviz_core
    ${DVZ_VK_COMPONENTS}
    # Temporary bridge: vklite surface code currently depends on window internals.
    # Window internals also pull input router symbols.
    datoviz_input
    datoviz_window
    datoviz_volk
)

target_compile_definitions(datoviz_vk_layer PUBLIC ${DVZ_COMPILE_DEFINITIONS})

target_include_directories(datoviz_vk_layer
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

add_library(datoviz_canvas_layer SHARED "empty.c")
set_target_properties(datoviz_canvas_layer PROPERTIES OUTPUT_NAME "datoviz_canvas")

target_link_libraries(datoviz_canvas_layer
    PRIVATE
    datoviz_core
    datoviz_vk_layer
    ${DVZ_CANVAS_COMPONENTS}
    datoviz_volk
)

target_compile_definitions(datoviz_canvas_layer PUBLIC ${DVZ_COMPILE_DEFINITIONS})

target_include_directories(datoviz_canvas_layer
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

add_library(datoviz::core ALIAS datoviz_core)
add_library(datoviz::vk ALIAS datoviz_vk_layer)
add_library(datoviz::canvas ALIAS datoviz_canvas_layer)

install(TARGETS datoviz_core datoviz_vk_layer datoviz_canvas_layer
    LIBRARY DESTINATION lib
)

# -------------------------------------------------------------------------------------------------
# Aggregate target
# -------------------------------------------------------------------------------------------------
#
target_link_libraries(datoviz
    PRIVATE
    ${DVZ_COMPONENTS}
    datoviz_volk
)

target_include_directories(datoviz
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

add_library(datoviz::datoviz ALIAS datoviz)

if(DVZ_THREADING_TARGET)
    target_link_libraries(datoviz_core PRIVATE ${DVZ_THREADING_TARGET})
    target_link_libraries(datoviz PRIVATE ${DVZ_THREADING_TARGET})
endif()

if(DVZ_HAS_GLFW AND TARGET glfw)
    target_link_libraries(datoviz_canvas_layer PRIVATE glfw)
    target_link_libraries(datoviz PRIVATE glfw)
endif()

if(DVZ_WITH_MIMALLOC)
    target_link_libraries(datoviz_core PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
    target_link_libraries(datoviz PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
endif()

if(APPLE)
    foreach(_dvz_layer datoviz_core datoviz_vk_layer datoviz_canvas_layer)
        if(TARGET ${_dvz_layer})
            set_target_properties(${_dvz_layer} PROPERTIES
                BUILD_RPATH "@loader_path"
                INSTALL_RPATH "@loader_path"
            )
        endif()
    endforeach()
endif()

foreach(_dvz_target datoviz datoviz_core datoviz_vk_layer datoviz_canvas_layer ${DVZ_COMPONENTS})
    if(TARGET ${_dvz_target})
        set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS ${_dvz_target})
    endif()
endforeach()

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

    foreach(target_name
        datoviz
        datoviz_core
        datoviz_vk_layer
        datoviz_canvas_layer
        datoviz_common
        datoviz_ds
        datoviz_fileio
        datoviz_math
        datoviz_thread
        datoviz_video
        datoviz_vk
        datoviz_vklite)
        if(TARGET ${target_name})
            # Always add sanitizer compile flags in Debug.
            target_compile_options(${target_name} PRIVATE $<$<CONFIG:Debug>:${DVZ_SANITIZER_FLAGS}>)

            # MSVC linker does not recognize /fsanitize=address; let the compiler drive ASan.
            if(NOT CC_MSVC)
                target_link_options(${target_name} PRIVATE $<$<CONFIG:Debug>:${DVZ_SANITIZER_FLAGS}>)
            endif()
        endif()
    endforeach()
endif()

# Ensure consistent C++ STL iterator debugging level across all C++ TUs on MSVC to
# avoid LNK2038 mismatches like 'annotate_string'/'annotate_vector'.
if(CC_MSVC)
    list(APPEND DVZ_COMPILE_DEFINITIONS
        _ITERATOR_DEBUG_LEVEL=$<IF:$<CONFIG:Debug>,1,0>
        _HAS_ITERATOR_DEBUGGING=$<IF:$<CONFIG:Debug>,1,0>
    )
endif()

set(DVZ_SANITIZER_FLAGS "${DVZ_SANITIZER_FLAGS}" CACHE INTERNAL "Sanitizer flags for Debug targets" FORCE)
set(DVZ_ACTIVE_SANITIZERS "${DVZ_ACTIVE_SANITIZERS}" CACHE INTERNAL "Enabled sanitizer names" FORCE)
