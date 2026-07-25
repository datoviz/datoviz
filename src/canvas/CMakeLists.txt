file(GLOB CANVAS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_canvas OBJECT ${CANVAS_SRC})

target_include_directories(datoviz_canvas
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/src/window
    ${PROJECT_SOURCE_DIR}/src/canvas
    ${PROJECT_SOURCE_DIR}/src/vk
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/volk
)

target_compile_definitions(datoviz_canvas PUBLIC ${DVZ_COMPILE_DEFINITIONS})
if(WIN32)
    target_compile_definitions(datoviz_canvas PRIVATE DVZ_SHARED)
endif()

set(DVZ_CANVAS_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
file(GLOB DVZ_CANVAS_SHADER_SOURCES
    "${DVZ_CANVAS_SHADER_DIR}/*.vert"
    "${DVZ_CANVAS_SHADER_DIR}/*.frag"
)

if(DVZ_CANVAS_SHADER_SOURCES)
    find_package(Python COMPONENTS Interpreter REQUIRED)

    if(NOT GLSLC)
        message(FATAL_ERROR "glslc is required to compile Canvas shaders; set GLSLC or DVZ_GLSLC.")
    endif()

    set(DVZ_CANVAS_SHADER_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    file(MAKE_DIRECTORY ${DVZ_CANVAS_SHADER_OUT_DIR})

    set(DVZ_CANVAS_SHADER_HEADERS "")

    foreach(_dvz_shader ${DVZ_CANVAS_SHADER_SOURCES})
        get_filename_component(_dvz_shader_name ${_dvz_shader} NAME_WE)
        get_filename_component(_dvz_shader_ext ${_dvz_shader} EXT)
        if(_dvz_shader_ext STREQUAL ".vert")
            set(_dvz_shader_stage vertex)
        else()
            set(_dvz_shader_stage fragment)
        endif()
        string(REPLACE "." "_" _dvz_shader_ext_tag ${_dvz_shader_ext})
        set(_dvz_shader_base "${_dvz_shader_name}${_dvz_shader_ext_tag}")
        set(_dvz_shader_spv "${DVZ_CANVAS_SHADER_OUT_DIR}/${_dvz_shader_base}.spv")
        set(_dvz_shader_header "${DVZ_CANVAS_SHADER_OUT_DIR}/${_dvz_shader_base}_spirv.h")

        dvz_compile_glsl(
            SOURCE "${_dvz_shader}"
            OUTPUT "${_dvz_shader_spv}"
            STAGE "${_dvz_shader_stage}"
            PROFILE graphics)

        add_custom_command(
            OUTPUT ${_dvz_shader_header}
            COMMAND ${Python_EXECUTABLE}
            ${PROJECT_SOURCE_DIR}/tools/spirv_to_c.py
            --input
            ${_dvz_shader_spv}
            --output
            ${_dvz_shader_header}
            --symbol
            dvz_canvas_${_dvz_shader_base}_spv
            DEPENDS ${_dvz_shader_spv} ${PROJECT_SOURCE_DIR}/tools/spirv_to_c.py
            COMMENT "Embedding Canvas shader ${_dvz_shader_name}"
            VERBATIM
        )

        set_source_files_properties(${_dvz_shader_header} PROPERTIES GENERATED TRUE)

        list(APPEND DVZ_CANVAS_SHADER_HEADERS ${_dvz_shader_header})
    endforeach()

    add_custom_target(datoviz_canvas_shaders DEPENDS ${DVZ_CANVAS_SHADER_HEADERS})
    add_dependencies(datoviz_canvas datoviz_canvas_shaders)
    target_sources(datoviz_canvas PRIVATE ${DVZ_CANVAS_SHADER_HEADERS})
    target_include_directories(datoviz_canvas PRIVATE ${DVZ_CANVAS_SHADER_OUT_DIR})
endif()
