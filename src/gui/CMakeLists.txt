# -------------------------------------------------------------------------------------------------
# Dear ImGui overlay
# -------------------------------------------------------------------------------------------------

set(GUI_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/gui.cpp"
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/imgui.cpp"
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/imgui_demo.cpp"
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/imgui_draw.cpp"
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/imgui_tables.cpp"
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/imgui_widgets.cpp"
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/backends/imgui_impl_glfw.cpp"
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/backends/imgui_impl_vulkan.cpp"
)

set(GUI_EXTERNAL_SRC
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/imgui.cpp"
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/imgui_demo.cpp"
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/imgui_draw.cpp"
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/imgui_tables.cpp"
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/imgui_widgets.cpp"
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/backends/imgui_impl_glfw.cpp"
    "${PROJECT_SOURCE_DIR}/external/cimgui/imgui/backends/imgui_impl_vulkan.cpp"
)

set(CIMGUI_SRC
    "${PROJECT_SOURCE_DIR}/external/cimgui/cimgui.cpp"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set_source_files_properties(${GUI_EXTERNAL_SRC} PROPERTIES COMPILE_FLAGS "/W0")
    set_source_files_properties(${CIMGUI_SRC} PROPERTIES COMPILE_FLAGS "/W0")
else()
    set_source_files_properties(${GUI_EXTERNAL_SRC} PROPERTIES COMPILE_FLAGS "-w")
    set_source_files_properties(${CIMGUI_SRC} PROPERTIES COMPILE_FLAGS "-w")
endif()

add_library(datoviz_gui OBJECT ${GUI_SRC})
add_library(datoviz_cimgui OBJECT ${CIMGUI_SRC})

target_include_directories(datoviz_gui
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/gui
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/cimgui/imgui
    ${PROJECT_SOURCE_DIR}/external/cimgui/imgui/backends
    ${PROJECT_SOURCE_DIR}/external/volk
)

target_link_libraries(datoviz_gui
    PUBLIC
    datoviz_controller
    datoviz_scene
    datoviz_canvas
    datoviz_window
    datoviz_vklite
    datoviz_vk
    datoviz_volk
    glfw::glfw
)

target_compile_definitions(datoviz_gui PUBLIC
    ${DVZ_COMPILE_DEFINITIONS}
    IMGUI_IMPL_VULKAN_USE_VOLK
    IM_COUNTOF=IM_ARRAYSIZE
    DVZ_HAS_GUI=1
)

target_include_directories(datoviz_cimgui
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/external/cimgui
    ${PROJECT_SOURCE_DIR}/external/cimgui/imgui
)

target_compile_definitions(datoviz_cimgui PUBLIC
    ${DVZ_COMPILE_DEFINITIONS}
    IM_COUNTOF=IM_ARRAYSIZE
    DVZ_HAS_GUI=1
)

if(WIN32)
    target_compile_definitions(datoviz_gui PRIVATE DVZ_SHARED)
    target_compile_definitions(datoviz_cimgui PRIVATE DVZ_SHARED)
endif()
