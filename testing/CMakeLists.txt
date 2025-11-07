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

if(NOT DVZ_ENABLE_CUDA)
    list(FILTER TEST_SOURCES EXCLUDE REGEX "/src/video/tests/test_video\\.c.*")
endif()

set(DVZ_TEST_SPIRV_DIR "${PROJECT_BINARY_DIR}/testing/spirv")
set(DVZ_TEST_SHADER_DIR "${PROJECT_SOURCE_DIR}/src/vklite/tests/shaders")
file(GLOB DVZ_VKLITE_TEST_SHADERS
    "${DVZ_TEST_SHADER_DIR}/*.vert"
    "${DVZ_TEST_SHADER_DIR}/*.frag"
    "${DVZ_TEST_SHADER_DIR}/*.comp"
)

if(DVZ_VKLITE_TEST_SHADERS)
    file(MAKE_DIRECTORY "${DVZ_TEST_SPIRV_DIR}")

    find_program(DVZ_GLSLC_EXECUTABLE
        NAMES glslc
        HINTS
        "$ENV{VULKAN_SDK}/bin"
        "$ENV{VULKAN_SDK}/Bin"
        "${PROJECT_SOURCE_DIR}/bin/vulkan/linux"
        "${PROJECT_SOURCE_DIR}/bin/vulkan/macos"
        "${PROJECT_SOURCE_DIR}/bin/vulkan/windows"
    )

    if(NOT DVZ_GLSLC_EXECUTABLE)
        message(FATAL_ERROR "glslc is required to compile vklite test shaders.")
    endif()

    set(DVZ_VKLITE_TEST_SPIRV_OUTPUTS)

    foreach(_dvz_shader ${DVZ_VKLITE_TEST_SHADERS})
        get_filename_component(_dvz_shader_name "${_dvz_shader}" NAME)
        set(_dvz_shader_output "${DVZ_TEST_SPIRV_DIR}/${_dvz_shader_name}.spv")
        add_custom_command(
            OUTPUT "${_dvz_shader_output}"
            COMMAND ${DVZ_GLSLC_EXECUTABLE} -o "${_dvz_shader_output}" "${_dvz_shader}"
            DEPENDS "${_dvz_shader}"
            COMMENT "Compiling ${_dvz_shader_name} to SPIR-V"
            VERBATIM
        )
        list(APPEND DVZ_VKLITE_TEST_SPIRV_OUTPUTS "${_dvz_shader_output}")
    endforeach()

    add_custom_target(vklite_test_shaders DEPENDS ${DVZ_VKLITE_TEST_SPIRV_OUTPUTS})
endif()

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
    datoviz_video
    datoviz_vk
    datoviz_vklite
    datoviz_volk
    testing
)

if(DVZ_THREADING_TARGET)
    target_link_libraries(dvztest PRIVATE ${DVZ_THREADING_TARGET})
endif()

if(TARGET vklite_test_shaders)
    add_dependencies(dvztest vklite_test_shaders)
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
    target_include_directories(dvztest PRIVATE ${PROJECT_SOURCE_DIR}/external)
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

target_compile_definitions(dvztest PRIVATE DVZ_TEST_SPIRV_DIR="${DVZ_TEST_SPIRV_DIR}")

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
