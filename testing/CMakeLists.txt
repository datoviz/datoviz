# -------------------------------------------------------------------------------------------------
# Generic testing CMake file
# -------------------------------------------------------------------------------------------------

add_library(testing STATIC
    testing.cpp
    datoviz_testing.c
)

get_property(_dvz_compile_definitions GLOBAL PROPERTY DVZ_COMPILE_DEFINITIONS)

if(DVZ_HAS_CUDA)
    find_package(CUDAToolkit REQUIRED)

    if(NOT CMAKE_CUDA_ARCHITECTURES)
        # Default to native architecture so CMake is satisfied when CUDA is enabled.
        set(CMAKE_CUDA_ARCHITECTURES "native")
    endif()

    set(DVZ_CUDA_ARCHS "${CMAKE_CUDA_ARCHITECTURES}")
    enable_language(CUDA)
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

if(APPLE)
    set(_dvz_runtime_rpath "@executable_path;@loader_path")
endif()

# -------------------------------------------------------------------------------------------------
# Synthetic scheduler test probe
# -------------------------------------------------------------------------------------------------

add_executable(dvztest_scheduler
    dvztest_scheduler.c
)

target_link_libraries(dvztest_scheduler
    PRIVATE
    datoviz_common
    testing
)

if(DVZ_THREADING_TARGET)
    target_link_libraries(dvztest_scheduler PRIVATE ${DVZ_THREADING_TARGET})
endif()

if(DVZ_WITH_MIMALLOC)
    target_link_libraries(dvztest_scheduler PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
endif()

target_include_directories(dvztest_scheduler
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/common
)

if(_dvz_compile_definitions)
    target_compile_definitions(dvztest_scheduler PRIVATE ${_dvz_compile_definitions})
endif()

if(APPLE)
    set_target_properties(dvztest_scheduler PROPERTIES
        BUILD_RPATH "${_dvz_runtime_rpath}"
        INSTALL_RPATH "${_dvz_runtime_rpath}"
    )
endif()

add_test(NAME dvztest_scheduler COMMAND dvztest_scheduler)

# -------------------------------------------------------------------------------------------------
# Datoviz test runner
# -------------------------------------------------------------------------------------------------

# All module C/C++ test sources.
file(GLOB DVZ_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/*/tests/*.c*"
)

# test_cuda.cu is wired manually in the CUDA block below; keep it out of generic test source groups.
list(FILTER DVZ_MODULE_TEST_SOURCES EXCLUDE REGEX "/src/vk/tests/test_cuda\\.cu$")

# Integration runner sources.
set(TEST_SOURCES
    ${DVZ_MODULE_TEST_SOURCES}
    "dvztest.c"
)

if(NOT DVZ_HAS_CUDA)
    list(FILTER DVZ_MODULE_TEST_SOURCES EXCLUDE REGEX "/src/video/tests/test_video_nvenc\\.c.*")
    list(FILTER TEST_SOURCES EXCLUDE REGEX "/src/video/tests/test_video_nvenc\\.c.*")
endif()

if(NOT DVZ_HAS_KVZ)
    list(FILTER DVZ_MODULE_TEST_SOURCES EXCLUDE REGEX "/src/video/tests/test_video_kvazaar\\.c.*")
    list(FILTER TEST_SOURCES EXCLUDE REGEX "/src/video/tests/test_video_kvazaar\\.c.*")
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

    set(_dvz_glslc_hints
        "$ENV{VULKAN_SDK}/bin"
        "$ENV{VULKAN_SDK}/Bin"
    )

    find_program(DVZ_GLSLC_EXECUTABLE
        NAMES glslc
        HINTS ${_dvz_glslc_hints}
    )

    if(NOT DVZ_GLSLC_EXECUTABLE)
        message(FATAL_ERROR "glslc is required to compile vklite test shaders.")
    endif()

    set(DVZ_VKLITE_TEST_SPIRV_OUTPUTS)

    foreach(_dvz_shader ${DVZ_VKLITE_TEST_SHADERS})
        get_filename_component(_dvz_shader_name "${_dvz_shader}" NAME)
        get_filename_component(_dvz_shader_ext "${_dvz_shader}" EXT)
        set(_dvz_shader_output "${DVZ_TEST_SPIRV_DIR}/${_dvz_shader_name}.spv")

        set(_dvz_shader_args)
        if(_dvz_shader_ext STREQUAL ".comp")
            list(APPEND _dvz_shader_args
                --target-env=vulkan1.3
                --target-spv=spv1.6
            )
        endif()

        add_custom_command(
            OUTPUT "${_dvz_shader_output}"
            COMMAND ${DVZ_GLSLC_EXECUTABLE}
            ${_dvz_shader_args}
            -o "${_dvz_shader_output}"
            "${_dvz_shader}"
            DEPENDS "${_dvz_shader}"
            COMMENT "Compiling ${_dvz_shader_name} to SPIR-V"
            VERBATIM
        )
        list(APPEND DVZ_VKLITE_TEST_SPIRV_OUTPUTS "${_dvz_shader_output}")
    endforeach()

    add_custom_target(vklite_test_shaders DEPENDS ${DVZ_VKLITE_TEST_SPIRV_OUTPUTS})
endif()

set(_dvz_has_core 0)
set(_dvz_has_controller 0)
set(_dvz_has_vk 0)
set(_dvz_has_canvas 0)
set(_dvz_has_drp2 0)
set(_dvz_has_scene 0)
set(_dvz_has_app 0)

if(TARGET datoviz_common
    AND TARGET datoviz_ds
    AND TARGET datoviz_fileio
    AND TARGET datoviz_geom
    AND TARGET datoviz_math
    AND TARGET datoviz_thread)
    set(_dvz_has_core 1)
endif()

if(TARGET datoviz_vk AND TARGET datoviz_vklite)
    set(_dvz_has_vk 1)
endif()

if(TARGET datoviz_controller)
    set(_dvz_has_controller 1)
endif()

if(TARGET datoviz_input AND TARGET datoviz_window AND TARGET datoviz_canvas AND TARGET datoviz_stream AND TARGET datoviz_video)
    set(_dvz_has_canvas 1)
endif()

if(TARGET datoviz_drp2)
    set(_dvz_has_drp2 1)
endif()

if(TARGET datoviz_scene)
    set(_dvz_has_scene 1)
endif()

if(TARGET datoviz_app)
    set(_dvz_has_app 1)
endif()

if(NOT _dvz_has_drp2)
    list(FILTER DVZ_MODULE_TEST_SOURCES EXCLUDE REGEX "/src/drp2/tests/")
    list(FILTER TEST_SOURCES EXCLUDE REGEX "/src/drp2/tests/")
endif()

if(NOT _dvz_has_controller)
    list(FILTER DVZ_MODULE_TEST_SOURCES EXCLUDE REGEX "/src/controller/tests/")
    list(FILTER TEST_SOURCES EXCLUDE REGEX "/src/controller/tests/")
endif()

if(NOT _dvz_has_scene)
    list(FILTER DVZ_MODULE_TEST_SOURCES EXCLUDE REGEX "/src/scene/tests/")
    list(FILTER TEST_SOURCES EXCLUDE REGEX "/src/scene/tests/")
endif()

set(DVZ_TEST_CORE_SOURCES ${DVZ_MODULE_TEST_SOURCES})
list(FILTER DVZ_TEST_CORE_SOURCES INCLUDE REGEX "/src/(common|ds|fileio|geom|math|thread)/tests/")

set(DVZ_TEST_COMMON_SOURCES ${DVZ_MODULE_TEST_SOURCES})
list(FILTER DVZ_TEST_COMMON_SOURCES INCLUDE REGEX "/src/common/tests/")

set(DVZ_TEST_DS_SOURCES ${DVZ_MODULE_TEST_SOURCES})
list(FILTER DVZ_TEST_DS_SOURCES INCLUDE REGEX "/src/ds/tests/")

set(DVZ_TEST_FILEIO_SOURCES ${DVZ_MODULE_TEST_SOURCES})
list(FILTER DVZ_TEST_FILEIO_SOURCES INCLUDE REGEX "/src/fileio/tests/")

set(DVZ_TEST_GEOM_SOURCES ${DVZ_MODULE_TEST_SOURCES})
list(FILTER DVZ_TEST_GEOM_SOURCES INCLUDE REGEX "/src/geom/tests/")

set(DVZ_TEST_MATH_SOURCES ${DVZ_MODULE_TEST_SOURCES})
list(FILTER DVZ_TEST_MATH_SOURCES INCLUDE REGEX "/src/math/tests/")

set(DVZ_TEST_THREAD_SOURCES ${DVZ_MODULE_TEST_SOURCES})
list(FILTER DVZ_TEST_THREAD_SOURCES INCLUDE REGEX "/src/thread/tests/")

set(DVZ_TEST_CONTROLLER_SOURCES ${DVZ_MODULE_TEST_SOURCES})
list(FILTER DVZ_TEST_CONTROLLER_SOURCES INCLUDE REGEX "/src/controller/tests/")

set(DVZ_TEST_DRP2_SOURCES ${DVZ_MODULE_TEST_SOURCES})
list(FILTER DVZ_TEST_DRP2_SOURCES INCLUDE REGEX "/src/drp2/tests/")

set(DVZ_TEST_SCENE_SOURCES ${DVZ_MODULE_TEST_SOURCES})
list(FILTER DVZ_TEST_SCENE_SOURCES INCLUDE REGEX "/src/scene/tests/")

set(DVZ_TEST_VK_SOURCES ${DVZ_MODULE_TEST_SOURCES})
list(FILTER DVZ_TEST_VK_SOURCES INCLUDE REGEX "/src/(vk|vklite)/tests/")
list(FILTER DVZ_TEST_VK_SOURCES EXCLUDE REGEX "/src/vklite/tests/test_present\\.c.*")
list(FILTER DVZ_TEST_VK_SOURCES EXCLUDE REGEX "/src/vklite/tests/test_vklite\\.c.*")

set(DVZ_TEST_CANVAS_SOURCES ${DVZ_MODULE_TEST_SOURCES})
list(FILTER DVZ_TEST_CANVAS_SOURCES INCLUDE REGEX "/src/(input|window|stream|video|canvas)/tests/")

set(DVZ_TEST_GUI_SOURCES ${DVZ_MODULE_TEST_SOURCES})
list(FILTER DVZ_TEST_GUI_SOURCES INCLUDE REGEX "/src/gui/tests/")

function(dvz_add_cpu_test_runner target source_var)
    add_executable(${target}
        ${${source_var}}
        components/${target}.c
    )

    target_link_libraries(${target}
        PRIVATE
        ${ARGN}
        testing
    )

    if(DVZ_THREADING_TARGET)
        target_link_libraries(${target} PRIVATE ${DVZ_THREADING_TARGET})
    endif()

    if(DVZ_WITH_MIMALLOC)
        target_link_libraries(${target} PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
    endif()

    target_include_directories(${target}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/common
    )

    if(_dvz_compile_definitions)
        target_compile_definitions(${target} PRIVATE ${_dvz_compile_definitions})
    endif()

    if(APPLE)
        set_target_properties(${target} PROPERTIES
            BUILD_RPATH "${_dvz_runtime_rpath}"
            INSTALL_RPATH "${_dvz_runtime_rpath}"
        )
    endif()

    add_test(NAME ${target} COMMAND ${target})
endfunction()

if(_dvz_has_core)
    dvz_add_cpu_test_runner(
        dvztest_common
        DVZ_TEST_COMMON_SOURCES
        datoviz_common
    )

    dvz_add_cpu_test_runner(
        dvztest_ds
        DVZ_TEST_DS_SOURCES
        datoviz_common
        datoviz_ds
    )

    dvz_add_cpu_test_runner(
        dvztest_fileio
        DVZ_TEST_FILEIO_SOURCES
        datoviz_common
        datoviz_fileio
    )

    dvz_add_cpu_test_runner(
        dvztest_geom
        DVZ_TEST_GEOM_SOURCES
        datoviz_common
        datoviz_geom
        datoviz_math
    )

    dvz_add_cpu_test_runner(
        dvztest_math
        DVZ_TEST_MATH_SOURCES
        datoviz_common
        datoviz_math
    )

    dvz_add_cpu_test_runner(
        dvztest_thread
        DVZ_TEST_THREAD_SOURCES
        datoviz_common
        datoviz_thread
    )

    add_executable(dvztest_core
        ${DVZ_TEST_CORE_SOURCES}
        components/dvztest_core.c
    )

    target_link_libraries(dvztest_core
        PRIVATE
        datoviz_common
        datoviz_ds
        datoviz_fileio
        datoviz_geom
        datoviz_math
        datoviz_thread
        testing
    )

    if(DVZ_THREADING_TARGET)
        target_link_libraries(dvztest_core PRIVATE ${DVZ_THREADING_TARGET})
    endif()

    if(DVZ_WITH_MIMALLOC)
        target_link_libraries(dvztest_core PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
    endif()

    target_include_directories(dvztest_core
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/src/vk/tests
        ${PROJECT_SOURCE_DIR}/src/vklite
        ${PROJECT_SOURCE_DIR}/src/canvas
        ${PROJECT_SOURCE_DIR}/external/volk
    )

    if(_dvz_compile_definitions)
        target_compile_definitions(dvztest_core PRIVATE ${_dvz_compile_definitions})
    endif()

    if(APPLE)
        set_target_properties(dvztest_core PROPERTIES
            BUILD_RPATH "${_dvz_runtime_rpath}"
            INSTALL_RPATH "${_dvz_runtime_rpath}"
        )
    endif()

    add_test(NAME dvztest_core COMMAND dvztest_core)
else()
    message(STATUS "Skipping dvztest_core: core target set is not enabled.")
endif()

if(_dvz_has_core AND _dvz_has_controller)
    dvz_add_cpu_test_runner(
        dvztest_controller
        DVZ_TEST_CONTROLLER_SOURCES
        datoviz_common
        datoviz_math
        datoviz_input
        datoviz_controller
    )
else()
    message(STATUS "Skipping dvztest_controller: controller target set is not enabled.")
endif()

if(_dvz_has_core AND _dvz_has_drp2)
    add_executable(dvztest_drp2
        ${DVZ_TEST_DRP2_SOURCES}
        components/dvztest_drp2.c
    )

    target_link_libraries(dvztest_drp2
        PRIVATE
        datoviz_common
        datoviz_drp2
        testing
    )

    if(_dvz_has_vk)
        target_link_libraries(dvztest_drp2 PRIVATE datoviz_vk datoviz_vklite datoviz_volk)
    endif()

    if(DVZ_HAS_CUDA)
        target_include_directories(dvztest_drp2 PRIVATE ${CUDAToolkit_INCLUDE_DIRS})
        target_include_directories(dvztest_drp2 PRIVATE ${PROJECT_SOURCE_DIR}/external)
        target_link_libraries(dvztest_drp2 PRIVATE CUDA::cudart CUDA::cuda_driver)
        target_link_libraries(dvztest_drp2 PRIVATE cuda)
    endif()

    if(DVZ_WITH_MIMALLOC)
        target_link_libraries(dvztest_drp2 PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
    endif()

    target_include_directories(dvztest_drp2
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/src/vklite
    )

    if(_dvz_compile_definitions)
        target_compile_definitions(dvztest_drp2 PRIVATE ${_dvz_compile_definitions})
    endif()

    if(APPLE)
        set_target_properties(dvztest_drp2 PROPERTIES
            BUILD_RPATH "${_dvz_runtime_rpath}"
            INSTALL_RPATH "${_dvz_runtime_rpath}"
        )
    endif()

    add_test(NAME dvztest_drp2 COMMAND dvztest_drp2)

    add_executable(dvz_drp2_player
        dvz_drp2_player.c
    )

    target_link_libraries(dvz_drp2_player
        PRIVATE
        datoviz_common
        datoviz_drp2
    )

    if(_dvz_has_vk)
        target_link_libraries(dvz_drp2_player PRIVATE datoviz_vk datoviz_vklite datoviz_volk)
    endif()

    if(DVZ_WITH_MIMALLOC)
        target_link_libraries(dvz_drp2_player PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
    endif()

    target_include_directories(dvz_drp2_player
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/common
    )

    if(_dvz_compile_definitions)
        target_compile_definitions(dvz_drp2_player PRIVATE ${_dvz_compile_definitions})
    endif()

    if(APPLE)
        set_target_properties(dvz_drp2_player PROPERTIES
            BUILD_RPATH "${_dvz_runtime_rpath}"
            INSTALL_RPATH "${_dvz_runtime_rpath}"
        )
    endif()
else()
    message(STATUS "Skipping dvztest_drp2: drp2 target set is not enabled.")
endif()

if(_dvz_has_core AND _dvz_has_drp2 AND _dvz_has_scene)
    add_executable(dvztest_scene
        ${DVZ_TEST_SCENE_SOURCES}
        components/dvztest_scene.c
    )

    target_link_libraries(dvztest_scene
        PRIVATE
        datoviz_common
        datoviz_ds
        datoviz_fileio
        datoviz_geom
        datoviz_math
        datoviz_thread
        datoviz_input
        datoviz_controller
        datoviz_window
        datoviz_canvas
        datoviz_stream
        datoviz_video
        datoviz_drp2
        datoviz_scene
        testing
    )

    if(_dvz_has_app)
        target_link_libraries(dvztest_scene PRIVATE datoviz_app)
    endif()

    if(TARGET datoviz_gui)
        target_link_libraries(dvztest_scene PRIVATE datoviz_gui)
    endif()
    if(TARGET datoviz_cimgui)
        target_link_libraries(dvztest_scene PRIVATE datoviz_cimgui)
    endif()

    if(_dvz_has_vk)
        target_link_libraries(dvztest_scene PRIVATE datoviz_vk datoviz_vklite datoviz_volk)
    endif()

    if(DVZ_WITH_MIMALLOC)
        target_link_libraries(dvztest_scene PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
    endif()

    target_include_directories(dvztest_scene
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/src/canvas
        ${PROJECT_SOURCE_DIR}/external/volk
    )

    if(_dvz_compile_definitions)
        target_compile_definitions(dvztest_scene PRIVATE ${_dvz_compile_definitions})
    endif()

    if(APPLE)
        set_target_properties(dvztest_scene PROPERTIES
            BUILD_RPATH "${_dvz_runtime_rpath}"
            INSTALL_RPATH "${_dvz_runtime_rpath}"
        )
    endif()

    add_test(NAME dvztest_scene COMMAND dvztest_scene)
else()
    message(STATUS "Skipping dvztest_scene: scene target set is not enabled.")
endif()

if(_dvz_has_core AND _dvz_has_vk)
    add_executable(dvztest_vk
        ${DVZ_TEST_VK_SOURCES}
        components/dvztest_vk.c
    )

    target_link_libraries(dvztest_vk
        PRIVATE
        datoviz_common
        datoviz_ds
        datoviz_fileio
        datoviz_geom
        datoviz_math
        datoviz_thread
        datoviz_vk
        datoviz_vklite
        datoviz_volk
        testing
    )

    if(DVZ_THREADING_TARGET)
        target_link_libraries(dvztest_vk PRIVATE ${DVZ_THREADING_TARGET})
    endif()

    if(TARGET vklite_test_shaders)
        add_dependencies(dvztest_vk vklite_test_shaders)
    endif()

    if(DVZ_WITH_MIMALLOC)
        target_link_libraries(dvztest_vk PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
    endif()

    target_include_directories(dvztest_vk
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/src/vk/tests
        ${PROJECT_SOURCE_DIR}/src/vklite
        ${PROJECT_SOURCE_DIR}/src/canvas
        ${PROJECT_SOURCE_DIR}/external/volk
    )

    if(DVZ_HAS_CUDA)
        target_include_directories(dvztest_vk PRIVATE ${CUDAToolkit_INCLUDE_DIRS})
        target_include_directories(dvztest_vk PRIVATE ${PROJECT_SOURCE_DIR}/external)
        target_link_libraries(dvztest_vk PRIVATE CUDA::cudart CUDA::cuda_driver)
        target_link_libraries(dvztest_vk PRIVATE cuda)

        set(TEST_CUDA_KERNEL "${PROJECT_SOURCE_DIR}/src/vk/tests/test_cuda.cu")
        target_sources(dvztest_vk PRIVATE ${TEST_CUDA_KERNEL})
        set_source_files_properties(${TEST_CUDA_KERNEL} PROPERTIES LANGUAGE CUDA)
        set_target_properties(dvztest_vk PROPERTIES
            CUDA_ARCHITECTURES "${DVZ_CUDA_ARCHS}"
            CUDA_SEPARABLE_COMPILATION OFF
            CUDA_RESOLVE_DEVICE_SYMBOLS OFF
        )
    endif()

    if(_dvz_compile_definitions)
        target_compile_definitions(dvztest_vk PRIVATE ${_dvz_compile_definitions})
    endif()

    target_compile_definitions(dvztest_vk PRIVATE DVZ_HAS_GLFW=${DVZ_HAS_GLFW})
    target_compile_definitions(dvztest_vk PRIVATE DVZ_TEST_SPIRV_DIR="${DVZ_TEST_SPIRV_DIR}")

    if(APPLE)
        set_target_properties(dvztest_vk PROPERTIES
            BUILD_RPATH "${_dvz_runtime_rpath}"
            INSTALL_RPATH "${_dvz_runtime_rpath}"
        )
    endif()

    add_test(NAME dvztest_vk COMMAND dvztest_vk)
else()
    message(STATUS "Skipping dvztest_vk: vk target set is not enabled.")
endif()

if(_dvz_has_core AND _dvz_has_vk AND _dvz_has_canvas)
    add_executable(dvztest_canvas
        ${DVZ_TEST_CANVAS_SOURCES}
        components/dvztest_canvas.c
    )

    target_link_libraries(dvztest_canvas
        PRIVATE
        datoviz_common
        datoviz_ds
        datoviz_fileio
        datoviz_geom
        datoviz_math
        datoviz_thread
        datoviz_input
        datoviz_window
        datoviz_canvas
        datoviz_stream
        datoviz_video
        datoviz_vk
        datoviz_vklite
        datoviz_volk
        testing
    )

    if(DVZ_THREADING_TARGET)
        target_link_libraries(dvztest_canvas PRIVATE ${DVZ_THREADING_TARGET})
    endif()

    if(DVZ_WITH_MIMALLOC)
        target_link_libraries(dvztest_canvas PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
    endif()

    target_include_directories(dvztest_canvas
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/src/vk/tests
        ${PROJECT_SOURCE_DIR}/src/vklite
        ${PROJECT_SOURCE_DIR}/src/canvas
        ${PROJECT_SOURCE_DIR}/external/volk
    )

    if(_dvz_compile_definitions)
        target_compile_definitions(dvztest_canvas PRIVATE ${_dvz_compile_definitions})
    endif()

    target_compile_definitions(dvztest_canvas PRIVATE DVZ_HAS_GLFW=${DVZ_HAS_GLFW})
    target_compile_definitions(dvztest_canvas PRIVATE DVZ_TEST_SPIRV_DIR="${DVZ_TEST_SPIRV_DIR}")

    if(APPLE)
        set_target_properties(dvztest_canvas PROPERTIES
            BUILD_RPATH "${_dvz_runtime_rpath}"
            INSTALL_RPATH "${_dvz_runtime_rpath}"
        )
    endif()

    add_test(NAME dvztest_canvas COMMAND dvztest_canvas)
else()
    message(STATUS "Skipping dvztest_canvas: canvas target set is not enabled.")
endif()

if(_dvz_has_core AND _dvz_has_vk AND _dvz_has_canvas)
    if(_dvz_has_drp2 AND _dvz_has_scene AND _dvz_has_app AND TARGET datoviz_gui AND TARGET datoviz_cimgui)
        add_executable(dvztest_gui
            ${DVZ_TEST_GUI_SOURCES}
            components/dvztest_gui.c
        )

        target_link_libraries(dvztest_gui
            PRIVATE
            datoviz_common
            datoviz_ds
            datoviz_fileio
            datoviz_geom
            datoviz_math
            datoviz_thread
            datoviz_input
            datoviz_controller
            datoviz_window
            datoviz_canvas
            datoviz_stream
            datoviz_video
            datoviz_vk
            datoviz_vklite
            datoviz_drp2
            datoviz_scene
            datoviz_app
            datoviz_gui
            datoviz_cimgui
            testing
        )

        if(DVZ_THREADING_TARGET)
            target_link_libraries(dvztest_gui PRIVATE ${DVZ_THREADING_TARGET})
        endif()

        if(DVZ_WITH_MIMALLOC)
            target_link_libraries(dvztest_gui PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
        endif()

        target_include_directories(dvztest_gui
            PRIVATE
            ${PROJECT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${PROJECT_SOURCE_DIR}/src/common
            ${PROJECT_SOURCE_DIR}/src/canvas
            ${PROJECT_SOURCE_DIR}/external/volk
        )

        if(_dvz_compile_definitions)
            target_compile_definitions(dvztest_gui PRIVATE ${_dvz_compile_definitions})
        endif()

        target_compile_definitions(dvztest_gui PRIVATE DVZ_HAS_GLFW=${DVZ_HAS_GLFW})

        if(APPLE)
            set_target_properties(dvztest_gui PROPERTIES
                BUILD_RPATH "${_dvz_runtime_rpath}"
                INSTALL_RPATH "${_dvz_runtime_rpath}"
            )
        endif()

        add_test(NAME dvztest_gui COMMAND dvztest_gui)
    else()
        message(STATUS "Skipping dvztest_gui: GUI target set is not enabled.")
    endif()

    set(DVZ_INTEGRATION_SOURCES
        ${DVZ_MODULE_TEST_SOURCES}
        components/dvztest_integration.c
    )

    # Build one executable: dvztest
    add_executable(dvztest ${TEST_SOURCES})
    add_executable(dvztest_integration ${DVZ_INTEGRATION_SOURCES})

# Link to the common module and the shared testing library
    target_link_libraries(dvztest
        PRIVATE
        datoviz_common
        datoviz_ds
        datoviz_fileio
        datoviz_geom
        datoviz_math
        datoviz_thread
        datoviz_input
        datoviz_window
        datoviz_canvas
        datoviz_stream
        datoviz_video
        datoviz_vk
        datoviz_vklite
        testing
    )
    target_link_libraries(dvztest_integration
        PRIVATE
        datoviz_common
        datoviz_ds
        datoviz_fileio
        datoviz_geom
        datoviz_math
        datoviz_thread
        datoviz_input
        datoviz_window
        datoviz_canvas
        datoviz_stream
        datoviz_video
        datoviz_vk
        datoviz_vklite
        testing
    )

    if(TARGET datoviz_drp2)
        target_link_libraries(dvztest PRIVATE datoviz_drp2)
        target_link_libraries(dvztest_integration PRIVATE datoviz_drp2)
    endif()

    if(TARGET datoviz_controller)
        target_link_libraries(dvztest PRIVATE datoviz_controller)
        target_link_libraries(dvztest_integration PRIVATE datoviz_controller)
    endif()

    if(TARGET datoviz_scene)
        target_link_libraries(dvztest PRIVATE datoviz_scene)
        target_link_libraries(dvztest_integration PRIVATE datoviz_scene)
    endif()

    if(TARGET datoviz_app)
        target_link_libraries(dvztest PRIVATE datoviz_app)
        target_link_libraries(dvztest_integration PRIVATE datoviz_app)
    endif()

    if(TARGET datoviz_gui)
        target_link_libraries(dvztest PRIVATE datoviz_gui)
        target_link_libraries(dvztest_integration PRIVATE datoviz_gui)
    endif()
    if(TARGET datoviz_cimgui)
        target_link_libraries(dvztest PRIVATE datoviz_cimgui)
        target_link_libraries(dvztest_integration PRIVATE datoviz_cimgui)
    endif()

    if(NOT TARGET datoviz_gui)
        target_link_libraries(dvztest PRIVATE datoviz_volk)
        target_link_libraries(dvztest_integration PRIVATE datoviz_volk)
    endif()

    if(DVZ_THREADING_TARGET)
        target_link_libraries(dvztest PRIVATE ${DVZ_THREADING_TARGET})
        target_link_libraries(dvztest_integration PRIVATE ${DVZ_THREADING_TARGET})
    endif()

    if(TARGET vklite_test_shaders)
        add_dependencies(dvztest vklite_test_shaders)
        add_dependencies(dvztest_integration vklite_test_shaders)
    endif()

    if(DVZ_WITH_MIMALLOC)
        target_link_libraries(dvztest PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
        target_link_libraries(dvztest_integration PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
    endif()

    # Build the interactive canvas smoke app: dvz_live_canvas
    add_executable(dvz_live_canvas dvz_live_canvas.c)

    target_link_libraries(dvz_live_canvas
        PRIVATE
        datoviz_common
        datoviz_ds
        datoviz_fileio
        datoviz_geom
        datoviz_math
        datoviz_thread
        datoviz_input
        datoviz_window
        datoviz_canvas
        datoviz_stream
        datoviz_video
        datoviz_vk
        datoviz_vklite
        datoviz_drp2
        datoviz_scene
    )

    if(TARGET datoviz_app)
        target_link_libraries(dvz_live_canvas PRIVATE datoviz_app)
    endif()

    if(TARGET datoviz_controller)
        target_link_libraries(dvz_live_canvas PRIVATE datoviz_controller)
    endif()

    if(TARGET datoviz_gui)
        target_link_libraries(dvz_live_canvas PRIVATE datoviz_gui)
    endif()
    if(TARGET datoviz_cimgui)
        target_link_libraries(dvz_live_canvas PRIVATE datoviz_cimgui)
    endif()

    if(DVZ_THREADING_TARGET)
        target_link_libraries(dvz_live_canvas PRIVATE ${DVZ_THREADING_TARGET})
    endif()

    if(DVZ_WITH_MIMALLOC)
        target_link_libraries(dvz_live_canvas PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
    endif()

    target_include_directories(dvz_live_canvas
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/src/canvas
        ${PROJECT_SOURCE_DIR}/external/volk
    )

    if(_dvz_compile_definitions)
        target_compile_definitions(dvz_live_canvas PRIVATE ${_dvz_compile_definitions})
    endif()

    target_compile_definitions(dvz_live_canvas PRIVATE DVZ_HAS_GLFW=${DVZ_HAS_GLFW})

    # Include test headers and internal paths
    target_include_directories(dvztest
        PRIVATE

        # ${CMAKE_CURRENT_SOURCE_DIR}/tests
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/src/vk/tests
        ${PROJECT_SOURCE_DIR}/src/vklite
        ${PROJECT_SOURCE_DIR}/src/canvas
        ${PROJECT_SOURCE_DIR}/external/volk
    )
    target_include_directories(dvztest_integration
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/src/vk/tests
        ${PROJECT_SOURCE_DIR}/src/vklite
        ${PROJECT_SOURCE_DIR}/src/canvas
        ${PROJECT_SOURCE_DIR}/external/volk
    )

    if(APPLE)
        # Let executables resolve bundled dylibs located next to them.
        set_target_properties(dvztest PROPERTIES
            BUILD_RPATH "${_dvz_runtime_rpath}"
            INSTALL_RPATH "${_dvz_runtime_rpath}"
        )
        set_target_properties(dvztest_integration PROPERTIES
            BUILD_RPATH "${_dvz_runtime_rpath}"
            INSTALL_RPATH "${_dvz_runtime_rpath}"
        )
        set_target_properties(dvz_live_canvas PROPERTIES
            BUILD_RPATH "${_dvz_runtime_rpath}"
            INSTALL_RPATH "${_dvz_runtime_rpath}"
        )
    endif()

    if(DVZ_HAS_CUDA)
        set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler -Wno-unknown-warning-option")

        target_include_directories(dvztest PRIVATE ${CUDAToolkit_INCLUDE_DIRS})
        target_include_directories(dvztest PRIVATE ${PROJECT_SOURCE_DIR}/external)
        target_include_directories(dvztest_integration PRIVATE ${CUDAToolkit_INCLUDE_DIRS})
        target_include_directories(dvztest_integration PRIVATE ${PROJECT_SOURCE_DIR}/external)

        target_link_libraries(dvztest PRIVATE CUDA::cudart CUDA::cuda_driver)
        target_link_libraries(dvztest PRIVATE cuda)
        target_link_libraries(dvztest_integration PRIVATE CUDA::cudart CUDA::cuda_driver)
        target_link_libraries(dvztest_integration PRIVATE cuda)

        if(DVZ_NVENC_LIB_DIR)
            target_link_directories(dvztest PRIVATE "${DVZ_NVENC_LIB_DIR}")
            target_link_directories(dvztest_integration PRIVATE "${DVZ_NVENC_LIB_DIR}")
            # Ensure the loader picks NVENC straight from the repo tree when running tests.
            set_property(TARGET dvztest APPEND PROPERTY BUILD_RPATH "${DVZ_NVENC_LIB_DIR}")
            set_property(TARGET dvztest_integration APPEND PROPERTY BUILD_RPATH "${DVZ_NVENC_LIB_DIR}")
        endif()

        if(DVZ_NVENC_LIB)
            message(STATUS "Linking NVENC library: ${DVZ_NVENC_LIB}")
            target_link_libraries(dvztest PRIVATE ${DVZ_NVENC_LIB})
            target_link_libraries(dvztest_integration PRIVATE ${DVZ_NVENC_LIB})
        else()
            message(STATUS "DVZ_HAS_CUDA is ON but DVZ_NVENC_LIB is unset; NVENC tests will rely on runtime availability checks.")
        endif()

        # Optional: explicitly add the CUDA kernel file if it exists
        set(TEST_CUDA_KERNEL
            "${PROJECT_SOURCE_DIR}/src/vk/tests/test_cuda.cu"
        )
        set_target_properties(dvztest PROPERTIES CUDA_ARCHITECTURES "${DVZ_CUDA_ARCHS}")
        set_target_properties(dvztest_integration PROPERTIES CUDA_ARCHITECTURES "${DVZ_CUDA_ARCHS}")
        message(STATUS "Adding CUDA test kernel: ${TEST_CUDA_KERNEL}")
        target_sources(dvztest PRIVATE ${TEST_CUDA_KERNEL})
        target_sources(dvztest_integration PRIVATE ${TEST_CUDA_KERNEL})
        set_source_files_properties(${TEST_CUDA_KERNEL} PROPERTIES LANGUAGE CUDA)

        # Avoid slow host-device link step for simple kernels
        set_target_properties(dvztest PROPERTIES
            CUDA_SEPARABLE_COMPILATION OFF
            CUDA_RESOLVE_DEVICE_SYMBOLS OFF
        )
        set_target_properties(dvztest_integration PROPERTIES
            CUDA_SEPARABLE_COMPILATION OFF
            CUDA_RESOLVE_DEVICE_SYMBOLS OFF
        )
    endif()

    # Ensure OS/Compiler macros are available to tests
    if(_dvz_compile_definitions)
        target_compile_definitions(dvztest PRIVATE ${_dvz_compile_definitions})
        target_compile_definitions(dvztest_integration PRIVATE ${_dvz_compile_definitions})
    endif()

    target_compile_definitions(dvztest PRIVATE DVZ_HAS_GLFW=${DVZ_HAS_GLFW})
    target_compile_definitions(dvztest_integration PRIVATE DVZ_HAS_GLFW=${DVZ_HAS_GLFW})
    target_compile_definitions(dvztest PRIVATE DVZ_TEST_SPIRV_DIR="${DVZ_TEST_SPIRV_DIR}")
    target_compile_definitions(
        dvztest_integration PRIVATE DVZ_TEST_SPIRV_DIR="${DVZ_TEST_SPIRV_DIR}")

    # Register it as a CTest entry
    add_test(NAME dvztest COMMAND dvztest)
    add_test(NAME dvztest_integration COMMAND dvztest_integration)
else()
    message(STATUS "Skipping dvztest/dvz_live_canvas: full core+vk+canvas target set is not enabled.")
endif()

# -------------------------------------------------------------------------------------------------
# Public header probe
# -------------------------------------------------------------------------------------------------
#
if(TARGET datoviz_core)
    add_executable(dvz_public_header_probe dvz_public_header_probe.c)
    add_executable(dvz_public_header_cpp_probe dvz_public_header_probe.cpp)

    target_link_libraries(dvz_public_header_probe
        PRIVATE
        datoviz_core
    )

    set_target_properties(dvz_public_header_probe PROPERTIES
        LINKER_LANGUAGE CXX # ensure C++ runtime/sanitizers are linked when headers pull C++ objects
    )

    target_include_directories(dvz_public_header_probe
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/external
    )
    target_include_directories(dvz_public_header_cpp_probe
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/external
        ${PROJECT_SOURCE_DIR}/external/volk
        ${PROJECT_SOURCE_DIR}/external/cglm/include
        ${PROJECT_SOURCE_DIR}/external/cimgui
        ${PROJECT_SOURCE_DIR}/external/msdf-atlas-gen/msdfgen
    )

    if(_dvz_compile_definitions)
        target_compile_definitions(dvz_public_header_probe PRIVATE ${_dvz_compile_definitions})
        target_compile_definitions(dvz_public_header_cpp_probe PRIVATE ${_dvz_compile_definitions})
    endif()

    if(APPLE)
        # Mirror the run-path configuration for the auxiliary test executable.
        set_target_properties(dvz_public_header_probe PROPERTIES
            BUILD_RPATH "${_dvz_runtime_rpath}"
            INSTALL_RPATH "${_dvz_runtime_rpath}"
        )
        set_target_properties(dvz_public_header_cpp_probe PROPERTIES
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
            "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:datoviz_core>:$ENV{LD_LIBRARY_PATH}"
            "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:datoviz_core>:$ENV{DYLD_LIBRARY_PATH}"
            "PATH=$<TARGET_FILE_DIR:datoviz_core>:$ENV{PATH}"
            $<TARGET_FILE:dvz_public_header_probe>
            WORKING_DIRECTORY $<TARGET_FILE_DIR:dvz_public_header_probe>
        )
    endif()
else()
    message(STATUS "Skipping public header probes: datoviz_core target is disabled.")
endif()

# Track testing targets so top-level logic can apply shared instrumentation (coverage, sanitizers, ...).
foreach(_dvz_test_target
    testing
    dvztest
    dvztest_core
    dvztest_drp2
    dvztest_scene
    dvztest_vk
    dvztest_canvas
    dvztest_integration
    dvz_live_canvas
    dvz_public_header_probe
    dvz_public_header_cpp_probe)
    if(TARGET ${_dvz_test_target})
        set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS ${_dvz_test_target})
    endif()
endforeach()

if(DVZ_SANITIZER_FLAGS)
    foreach(_target
        testing
        dvztest
        dvztest_core
        dvztest_drp2
        dvztest_scene
        dvztest_vk
        dvztest_canvas
        dvztest_integration
        dvz_live_canvas
        dvz_public_header_probe
        dvz_public_header_cpp_probe)
        if(TARGET ${_target})
            target_compile_options(${_target} PRIVATE
                $<$<AND:$<CONFIG:Debug>,$<NOT:$<COMPILE_LANGUAGE:CUDA>>>:${DVZ_SANITIZER_FLAGS}>
            )
            target_link_options(${_target} PRIVATE $<$<CONFIG:Debug>:${DVZ_SANITIZER_FLAGS}>)
        endif()
    endforeach()
endif()
