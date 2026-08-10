# Shared native GLSL-to-SPIR-V build helpers.

include_guard(GLOBAL)

option(DVZ_VALIDATE_SPIRV "Validate generated native SPIR-V with spirv-val" OFF)
option(
    DVZ_REQUIRE_PRECOMPILED_SHADERS
    "Fail configuration when glslc is unavailable for required native shader products"
    OFF)
option(
    DVZ_GLSLC_AUTO_DISCOVERY
    "Discover glslc from the Vulkan SDK and PATH when no explicit compiler is configured"
    ON)

set(DVZ_GLSLC_EXECUTABLE "" CACHE FILEPATH "Optional path to the glslc shader compiler")
if(DVZ_GLSLC_EXECUTABLE AND NOT EXISTS "${DVZ_GLSLC_EXECUTABLE}")
    message(
        FATAL_ERROR
        "Configured DVZ_GLSLC_EXECUTABLE does not exist: ${DVZ_GLSLC_EXECUTABLE}")
endif()
if(NOT DVZ_GLSLC_EXECUTABLE
        AND DEFINED ENV{DVZ_GLSLC}
        AND NOT "$ENV{DVZ_GLSLC}" STREQUAL "")
    if(NOT EXISTS "$ENV{DVZ_GLSLC}")
        message(
            FATAL_ERROR
            "Configured DVZ_GLSLC environment path does not exist: $ENV{DVZ_GLSLC}")
    endif()
    set(
        DVZ_GLSLC_EXECUTABLE
        "$ENV{DVZ_GLSLC}"
        CACHE FILEPATH "Optional path to the glslc shader compiler" FORCE)
endif()
if(NOT DVZ_GLSLC_EXECUTABLE AND DVZ_GLSLC_AUTO_DISCOVERY)
    find_program(
        _dvz_glslc
        NAMES glslc
        HINTS "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/Bin"
        NO_CACHE)
    if(_dvz_glslc)
        set(
            DVZ_GLSLC_EXECUTABLE
            "${_dvz_glslc}"
            CACHE FILEPATH "Optional path to the glslc shader compiler" FORCE)
    endif()
endif()
if(DVZ_GLSLC_EXECUTABLE)
    add_executable(dvz_glslc_tool IMPORTED)
    set_target_properties(dvz_glslc_tool PROPERTIES IMPORTED_LOCATION "${DVZ_GLSLC_EXECUTABLE}")
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
        HINTS "$ENV{VULKAN_SDK}/bin" "$ENV{VULKAN_SDK}/Bin"
        NO_CACHE)
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



function(dvz_validate_shader_configuration)
    if(DVZ_GLSLC_EXECUTABLE)
        return()
    endif()

    set(_dvz_required_products)
    if(DVZ_REQUIRE_PRECOMPILED_SHADERS)
        list(APPEND _dvz_required_products "DVZ_REQUIRE_PRECOMPILED_SHADERS=ON")
    endif()
    if(DVZ_BUILD_TESTING)
        list(APPEND _dvz_required_products "native test fixtures")
    endif()

    if(_dvz_required_products)
        list(JOIN _dvz_required_products ", " _dvz_required_products_text)
        message(
            FATAL_ERROR
            "glslc is required by ${_dvz_required_products_text}. Install it from the Vulkan SDK "
            "or a platform shaderc/glslc package, ensure it is on PATH, or set "
            "DVZ_GLSLC_EXECUTABLE to its full path.")
    endif()

    if(DVZ_BUILD_SCENE)
        if(NOT DVZ_HAS_SHADERC)
            message(
                FATAL_ERROR
                "DVZ_BUILD_SCENE=ON requires either glslc for embedded SPIR-V or runtime shaderc "
                "for the embedded-GLSL fallback. Install glslc from the Vulkan SDK or a platform "
                "shaderc/glslc package, set DVZ_GLSLC_EXECUTABLE, or enable an available shaderc "
                "provider with DVZ_ENABLE_SHADERC.")
        endif()
        message(
            WARNING
            "glslc not found — scene built-in shaders will use the runtime GLSL fallback. A "
            "compatible shaderc provider must be discoverable when the scene creates pipelines.")
    else()
        message(STATUS "glslc not found; no enabled Datoviz product requires it")
    endif()
endfunction()



function(dvz_compile_glsl)
    set(_one_value_args SOURCE OUTPUT STAGE PROFILE)
    set(_multi_value_args INCLUDE_DIRS DEPENDS)
    cmake_parse_arguments(DVZ_SHADER "" "${_one_value_args}" "${_multi_value_args}" ${ARGN})

    foreach(_required SOURCE OUTPUT STAGE PROFILE)
        if(NOT DVZ_SHADER_${_required})
            message(FATAL_ERROR "dvz_compile_glsl() requires ${_required}.")
        endif()
    endforeach()
    if(NOT TARGET dvz_glslc_tool)
        message(
            FATAL_ERROR
            "dvz_compile_glsl() requires glslc; set DVZ_GLSLC_EXECUTABLE or DVZ_GLSLC.")
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
            dvz_glslc_tool
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
