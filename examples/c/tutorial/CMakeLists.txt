cmake_minimum_required(VERSION 3.21)
project(datoviz_vklite_triangle C)

find_package(datoviz CONFIG REQUIRED)

add_executable(vklite_triangle vklite_triangle_spike.c)
target_link_libraries(vklite_triangle PRIVATE datoviz::datoviz)
target_compile_definitions(
    vklite_triangle
    PRIVATE
    DVZ_TUTORIAL_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)
