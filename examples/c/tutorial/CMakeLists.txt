cmake_minimum_required(VERSION 3.21)
project(datoviz_vklite_triangle C)

find_package(datoviz CONFIG REQUIRED)

function(add_tutorial_chapter target shader_dir use_vertex_buffer)
    add_executable(${target} triangle.c)
    target_link_libraries(${target} PRIVATE datoviz::datoviz)
    target_compile_definitions(
        ${target}
        PRIVATE
        DVZ_TUTORIAL_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/${shader_dir}"
        DVZ_TUTORIAL_USE_VERTEX_BUFFER=${use_vertex_buffer}
    )
endfunction()

add_tutorial_chapter(first_triangle shaders 0)
add_tutorial_chapter(shaders_and_pipeline shaders/pipeline 0)
add_tutorial_chapter(vertex_buffers shaders/vertex_buffer 1)

add_executable(indexed_depth_spike triangle.c)
target_link_libraries(indexed_depth_spike PRIVATE datoviz::datoviz)
target_compile_definitions(
    indexed_depth_spike
    PRIVATE
    DVZ_TUTORIAL_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders/depth"
    DVZ_TUTORIAL_USE_VERTEX_BUFFER=1
    DVZ_TUTORIAL_USE_INDEXED_DEPTH=1
)
