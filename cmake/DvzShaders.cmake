# Shared native GLSL-to-SPIR-V build helpers.

include_guard(GLOBAL)

option(DVZ_VALIDATE_SPIRV "Validate generated native SPIR-V with spirv-val" OFF)
option(
    DVZ_REQUIRE_PRECOMPILED_SHADERS
    "Fail configuration when glslc is unavailable for required native shader products"
    OFF)

set(GLSLC "${GLSLC}" CACHE FILEPATH "Path to the glslc shader compiler")
if(NOT GLSLC AND DEFINED ENV{DVZ_GLSLC} AND EXISTS "$ENV{DVZ_GLSLC}")
    set(GLSLC "$ENV{DVZ_GLSLC}" CACHE FILEPATH "Path to the glslc shader compiler" FORCE)
endif()
if(NOT GLSLC)
    find_program(
        _dvz_glslc
        NAMES glslc
        HINTS "${GLSLC_PATH}" "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/Bin")
    if(_dvz_glslc)
        set(GLSLC "${_dvz_glslc}" CACHE FILEPATH "Path to the glslc shader compiler" FORCE)
    endif()
endif()
if(GLSLC AND NOT EXISTS "${GLSLC}")
    message(FATAL_ERROR "Configured GLSLC path does not exist: ${GLSLC}")
endif()

set(DVZ_SPIRV_VAL_EXECUTABLE "" CACHE FILEPATH "Path to the spirv-val validator")
if(DEFINED ENV{DVZ_SPIRV_VAL} AND EXISTS "$ENV{DVZ_SPIRV_VAL}")
    set(
        DVZ_SPIRV_VAL_EXECUTABLE
        "$ENV{DVZ_SPIRV_VAL}"
        CACHE FILEPATH "Path to the spirv-val validator" FORCE)
endif()
if(DVZ_VALIDATE_SPIRV AND NOT DVZ_SPIRV_VAL_EXECUTABLE)
    find_program(
        _dvz_spirv_val
        NAMES spirv-val
        HINTS "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/Bin")
    if(_dvz_spirv_val)
        set(
            DVZ_SPIRV_VAL_EXECUTABLE
            "${_dvz_spirv_val}"
            CACHE FILEPATH "Path to the spirv-val validator" FORCE)
    endif()
endif()
if(DVZ_SPIRV_VAL_EXECUTABLE AND NOT EXISTS "${DVZ_SPIRV_VAL_EXECUTABLE}")
    message(
        FATAL_ERROR
        "Configured DVZ_SPIRV_VAL_EXECUTABLE path does not exist: "
        "${DVZ_SPIRV_VAL_EXECUTABLE}")
endif()
if(DVZ_VALIDATE_SPIRV AND NOT DVZ_SPIRV_VAL_EXECUTABLE)
    message(FATAL_ERROR "DVZ_VALIDATE_SPIRV=ON requires spirv-val.")
endif()

function(dvz_compile_glsl)
    set(_one_value_args SOURCE OUTPUT STAGE PROFILE)
    set(_multi_value_args INCLUDE_DIRS DEPENDS)
    cmake_parse_arguments(DVZ_SHADER "" "${_one_value_args}" "${_multi_value_args}" ${ARGN})

    foreach(_required SOURCE OUTPUT STAGE PROFILE)
        if(NOT DVZ_SHADER_${_required})
            message(FATAL_ERROR "dvz_compile_glsl() requires ${_required}.")
        endif()
    endforeach()
    if(NOT GLSLC)
        message(FATAL_ERROR "dvz_compile_glsl() requires glslc; set GLSLC or DVZ_GLSLC.")
    endif()

    if(DVZ_SHADER_PROFILE STREQUAL "graphics")
        set(_target_args --target-env=vulkan1.0 --target-spv=spv1.0)
        set(_validator_args --target-env vulkan1.0)
    elseif(DVZ_SHADER_PROFILE STREQUAL "compute")
        set(_target_args --target-env=vulkan1.3 --target-spv=spv1.6)
        set(_validator_args --target-env vulkan1.3)
    else()
        message(FATAL_ERROR "Unknown Datoviz shader profile: ${DVZ_SHADER_PROFILE}")
    endif()

    if(NOT DVZ_SHADER_STAGE MATCHES "^(vertex|fragment|compute)$")
        message(FATAL_ERROR "Unknown Datoviz shader stage: ${DVZ_SHADER_STAGE}")
    endif()

    set(_include_args)
    foreach(_include_dir IN LISTS DVZ_SHADER_INCLUDE_DIRS)
        list(APPEND _include_args "-I${_include_dir}")
    endforeach()

    get_filename_component(_output_dir "${DVZ_SHADER_OUTPUT}" DIRECTORY)
    set(_validate_command)
    if(DVZ_VALIDATE_SPIRV)
        list(
            APPEND
            _validate_command
            COMMAND
            "${DVZ_SPIRV_VAL_EXECUTABLE}"
            ${_validator_args}
            "${DVZ_SHADER_OUTPUT}")
    endif()

    get_filename_component(_shader_name "${DVZ_SHADER_SOURCE}" NAME)
    add_custom_command(
        OUTPUT "${DVZ_SHADER_OUTPUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_output_dir}"
        COMMAND
            "${GLSLC}"
            ${_target_args}
            "-fshader-stage=${DVZ_SHADER_STAGE}"
            ${_include_args}
            -o "${DVZ_SHADER_OUTPUT}"
            "${DVZ_SHADER_SOURCE}"
        ${_validate_command}
        DEPENDS "${DVZ_SHADER_SOURCE}" ${DVZ_SHADER_DEPENDS}
        COMMENT
            "Compiling ${_shader_name} with the Datoviz ${DVZ_SHADER_PROFILE} shader profile"
        VERBATIM)
endfunction()
