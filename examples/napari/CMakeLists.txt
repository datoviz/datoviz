# -------------------------------------------------------------------------------------------------
# Napari-oriented C demos
# -------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED PROJECT_SOURCE_DIR OR NOT TARGET datoviz_scene)
    message(FATAL_ERROR
        "examples/napari must be built as part of the datoviz source tree "
        "(add_subdirectory from the top-level CMakeLists.txt).")
endif()

function(dvz_add_napari_example name)
    add_executable(${name} ${name}.c)

    target_link_libraries(${name}
        PRIVATE
        datoviz_common
        datoviz_ds
        datoviz_fileio
        datoviz_math
        datoviz_thread
        datoviz_input
        datoviz_window
        datoviz_canvas
        datoviz_stream
        datoviz_video
        datoviz_drp2
        datoviz_scene
        datoviz_app
        datoviz_gui
    )

    if(TARGET datoviz_cimgui)
        target_link_libraries(${name} PRIVATE datoviz_cimgui)
    endif()

    if(TARGET datoviz_vklite)
        target_link_libraries(${name} PRIVATE datoviz_vk datoviz_vklite datoviz_volk)
    endif()

    if(DVZ_WITH_MIMALLOC)
        target_link_libraries(${name} PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
    endif()

    target_include_directories(${name}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/external/volk
    )

    get_property(_dvz_compile_definitions GLOBAL PROPERTY DVZ_COMPILE_DEFINITIONS)
    if(_dvz_compile_definitions)
        target_compile_definitions(${name} PRIVATE ${_dvz_compile_definitions})
    endif()

    if(TARGET datoviz_vklite)
        target_compile_definitions(${name} PRIVATE DVZ_DRP2_HAS_VKLITE=1)
    else()
        target_compile_definitions(${name} PRIVATE DVZ_DRP2_HAS_VKLITE=0)
    endif()

    if(APPLE)
        set_target_properties(${name} PROPERTIES
            BUILD_RPATH  "@executable_path;@loader_path"
            INSTALL_RPATH "@executable_path;@loader_path"
        )
    endif()

    set_target_properties(${name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/napari"
    )

    set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS ${name})
endfunction()

if(TARGET datoviz_app AND TARGET datoviz_gui AND DVZ_HAS_GLFW)
    dvz_add_napari_example(large_labels_segmentation_glfw)
endif()
