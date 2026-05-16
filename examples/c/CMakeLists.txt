# -------------------------------------------------------------------------------------------------
# C examples — built against the internal build tree
# -------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)

# Detect whether we're being built as part of the main datoviz tree.
if(NOT DEFINED PROJECT_SOURCE_DIR OR NOT TARGET datoviz_scene)
    message(FATAL_ERROR
        "examples/c must be built as part of the datoviz source tree "
        "(add_subdirectory from the top-level CMakeLists.txt).")
endif()

# Helper: add one C example executable.
#
#   dvz_add_example(name)
#
# Links against the same target set used by dvztest_scene.
function(dvz_add_example name)
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
    )

    if(TARGET datoviz_app)
        target_link_libraries(${name} PRIVATE datoviz_app)
    endif()

    if(TARGET datoviz_gui)
        target_link_libraries(${name} PRIVATE datoviz_gui)
    endif()
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
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/c"
    )

    set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS ${name})
endfunction()

# -------------------------------------------------------------------------------------------------
# Examples
# -------------------------------------------------------------------------------------------------

if(TARGET datoviz_app)
    dvz_add_example(hello_point)
    dvz_add_example(hello_path)
    dvz_add_example(hello_scatter)
    dvz_add_example(hello_triangle)
    dvz_add_example(hello_mesh)
    dvz_add_example(hello_field)
    dvz_add_example(hello_texture)
    dvz_add_example(record_scene_dvzr)
    if(DVZ_HAS_GLFW)
        dvz_add_example(replay_dvzr_glfw)
    endif()
    if(DVZ_HAS_GLFW)
        dvz_add_example(hello_mesh_glfw)
        dvz_add_example(hello_point_glfw)
        dvz_add_example(hello_pick_hover_glfw)
        dvz_add_example(hello_image_probe_glfw)
        dvz_add_example(hello_texture_update_glfw)
        dvz_add_example(hello_multi_panel_glfw)
        dvz_add_example(hello_linked_panels_glfw)
        dvz_add_example(hello_mesh_depth_peel_glfw)
        dvz_add_example(hosted_glfw_smoke)
        if(TARGET datoviz_gui)
            dvz_add_example(hello_mesh_wboit_glfw)
            dvz_add_example(hello_points_edl_glfw)
            dvz_add_example(showcase_lidar_glfw)
            dvz_add_example(hello_volume_glfw)
            dvz_add_example(hello_gui_glfw)
            if(TARGET datoviz_cimgui)
                dvz_add_example(hello_cimgui_glfw)
            endif()
            dvz_add_example(gui_viewport_glfw)
            dvz_add_example(gui_multi_viewport_glfw)
        endif()
    endif()
endif()

if(TARGET datoviz_drp2)
    dvz_add_example(raw_triangle)
    dvz_add_example(raw_triangle_drp2)
    if(TARGET datoviz_scene)
        dvz_add_example(export_scene_primitive_wgsl)
        dvz_add_example(export_scene_point_wgsl)
        dvz_add_example(export_scene_image_wgsl)
    endif()
endif()
