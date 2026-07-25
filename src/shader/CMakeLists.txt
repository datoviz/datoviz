# -------------------------------------------------------------------------------------------------
# Runtime shader compiler
# -------------------------------------------------------------------------------------------------

add_library(datoviz_shader_objects OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/shader.c")
add_library(datoviz_shader STATIC $<TARGET_OBJECTS:datoviz_shader_objects>)

target_include_directories(datoviz_shader_objects
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(
    datoviz_shader
    INTERFACE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(datoviz_shader_objects
    PUBLIC
    ${DVZ_COMPILE_DEFINITIONS}
    DVZ_HAS_SHADERC=${DVZ_HAS_SHADERC}
    PRIVATE
    DVZ_SHADERC_STATIC=${DVZ_SHADERC_STATIC}
)
target_compile_definitions(
    datoviz_shader INTERFACE ${DVZ_COMPILE_DEFINITIONS} DVZ_HAS_SHADERC=${DVZ_HAS_SHADERC})

target_link_libraries(datoviz_shader_objects PUBLIC datoviz_common)
target_link_libraries(datoviz_shader PUBLIC datoviz_common)

if(DVZ_HAS_SHADERC)
    if(DVZ_SHADERC_STATIC)
        target_link_libraries(datoviz_shader_objects PUBLIC ${DVZ_SHADERC_TARGET})
        target_link_libraries(datoviz_shader PUBLIC ${DVZ_SHADERC_TARGET})
    else()
        target_include_directories(datoviz_shader_objects PRIVATE "${DVZ_SHADERC_INCLUDE_DIR}")
        target_compile_definitions(
            datoviz_shader_objects PRIVATE DVZ_SHADERC_LIB_PATH="${DVZ_SHADERC_RUNTIME_LIBRARY}")
    endif()
endif()

if(NOT WIN32)
    target_link_libraries(datoviz_shader_objects PRIVATE dl)
    target_link_libraries(datoviz_shader PRIVATE dl)
endif()

if(DVZ_THREADING_TARGET)
    target_link_libraries(datoviz_shader_objects PUBLIC ${DVZ_THREADING_TARGET})
    target_link_libraries(datoviz_shader PUBLIC ${DVZ_THREADING_TARGET})
endif()

if(WIN32)
    target_compile_definitions(datoviz_shader_objects PRIVATE DVZ_SHARED)
endif()
