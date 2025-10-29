# -------------------------------------------------------------------------------------------------
# Generic testing CMake file
# -------------------------------------------------------------------------------------------------

add_library(testing STATIC testing.cpp)

get_property(_dvz_compile_definitions GLOBAL PROPERTY DVZ_COMPILE_DEFINITIONS)

if(DVZ_ENABLE_CUDA)
    find_package(CUDAToolkit REQUIRED)
endif()

target_include_directories(testing
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/common
)

# Ensure OS/Compiler macros are visible to the testing library as well
if(_dvz_compile_definitions)
    target_compile_definitions(testing PUBLIC ${_dvz_compile_definitions})
endif()

# Allow linking from C
set_target_properties(testing PROPERTIES
    LINKER_LANGUAGE CXX
)

# -------------------------------------------------------------------------------------------------
# Datoviz test runner
# -------------------------------------------------------------------------------------------------

# All C source files inside tests/
file(GLOB TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/*/tests/*.c*"
    "dvztest.c"
)

# Build one executable: dvztest
add_executable(dvztest ${TEST_SOURCES})

# Link to the common module and the shared testing library
target_link_libraries(dvztest
    PRIVATE
    datoviz_common
    datoviz_ds
    datoviz_fileio
    datoviz_math
    datoviz_thread
    datoviz_vk
    datoviz_vklite
    datoviz_volk
    testing
)

if(DVZ_THREADING_TARGET)
    target_link_libraries(dvztest PRIVATE ${DVZ_THREADING_TARGET})
endif()

if(DVZ_WITH_MIMALLOC)
    target_link_libraries(dvztest PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
endif()

# Track testing targets so top-level logic can apply shared instrumentation (coverage, sanitizers, ...).
foreach(_dvz_test_target testing dvztest dvz_public_header_probe)
    if(TARGET ${_dvz_test_target})
        set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS ${_dvz_test_target})
    endif()
endforeach()

# Include test headers and internal paths
target_include_directories(dvztest
    PRIVATE

    # ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/external/volk
)

if(APPLE)
    set(_dvz_runtime_rpath "@executable_path;@loader_path")
endif()

if(APPLE)
    # Let executables resolve bundled dylibs located next to them.
    set_target_properties(dvztest PROPERTIES
        BUILD_RPATH "${_dvz_runtime_rpath}"
        INSTALL_RPATH "${_dvz_runtime_rpath}"
    )
endif()

if(DVZ_ENABLE_CUDA)
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler -Wno-unknown-warning-option")

    target_include_directories(dvztest PRIVATE ${CUDAToolkit_INCLUDE_DIRS})
    target_link_libraries(dvztest PRIVATE CUDA::cudart CUDA::cuda_driver)

    # Enable CUDA language for dvztest so it can compile .cu kernels
    if(NOT CMAKE_CUDA_ARCHITECTURES)
        # Default to native architecture so CMake is satisfied when CUDA is enabled.
        set(CMAKE_CUDA_ARCHITECTURES "native")
    endif()
    enable_language(CUDA)

    # Optional: explicitly add the CUDA kernel file if it exists
    set(TEST_CUDA_KERNEL
        "${PROJECT_SOURCE_DIR}/src/vk/tests/test_cuda.cu"
    )
    set_target_properties(dvztest PROPERTIES CUDA_ARCHITECTURES "native")
    message(STATUS "Adding CUDA test kernel: ${TEST_CUDA_KERNEL}")
    target_sources(dvztest PRIVATE ${TEST_CUDA_KERNEL})
    set_source_files_properties(${TEST_CUDA_KERNEL} PROPERTIES LANGUAGE CUDA)

    # Avoid slow host-device link step for simple kernels
    set_target_properties(dvztest PROPERTIES
        CUDA_SEPARABLE_COMPILATION OFF
        CUDA_RESOLVE_DEVICE_SYMBOLS OFF
    )
endif()

# Ensure OS/Compiler macros are available to tests
if(_dvz_compile_definitions)
    target_compile_definitions(dvztest PRIVATE ${_dvz_compile_definitions})
endif()

# -------------------------------------------------------------------------------------------------
# Public header probe
# -------------------------------------------------------------------------------------------------
#
add_executable(dvz_public_header_probe dvz_public_header_probe.c)

target_link_libraries(dvz_public_header_probe
    PRIVATE
    datoviz
)

set_target_properties(dvz_public_header_probe PROPERTIES
    LINKER_LANGUAGE CXX # ensure C++ runtime/sanitizers are linked when headers pull C++ objects
)

target_include_directories(dvz_public_header_probe
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/external
)

if(_dvz_compile_definitions)
    target_compile_definitions(dvz_public_header_probe PRIVATE ${_dvz_compile_definitions})
endif()

if(APPLE)
    # Mirror the run-path configuration for the auxiliary test executable.
    set_target_properties(dvz_public_header_probe PROPERTIES
        BUILD_RPATH "${_dvz_runtime_rpath}"
        INSTALL_RPATH "${_dvz_runtime_rpath}"
    )
endif()

if(NOT WIN32)
    add_custom_command(TARGET dvz_public_header_probe
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env
        "ASAN_OPTIONS=detect_leaks=0"
        "LSAN_OPTIONS=detect_leaks=0"
        "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:datoviz>:$ENV{LD_LIBRARY_PATH}"
        "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:datoviz>:$ENV{DYLD_LIBRARY_PATH}"
        "PATH=$<TARGET_FILE_DIR:datoviz>:$ENV{PATH}"
        $<TARGET_FILE:dvz_public_header_probe>
        WORKING_DIRECTORY $<TARGET_FILE_DIR:dvz_public_header_probe>
    )
endif()

if(DVZ_SANITIZER_FLAGS)
    foreach(_target testing dvztest dvz_public_header_probe)
        if(TARGET ${_target})
            target_compile_options(${_target} PRIVATE
                $<$<AND:$<CONFIG:Debug>,$<NOT:$<COMPILE_LANGUAGE:CUDA>>>:${DVZ_SANITIZER_FLAGS}>
            )
            target_link_options(${_target} PRIVATE $<$<CONFIG:Debug>:${DVZ_SANITIZER_FLAGS}>)
        endif()
    endforeach()
endif()

# Register it as a CTest entry
add_test(NAME dvztest COMMAND dvztest)
