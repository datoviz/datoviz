# -------------------------------------------------------------------------------------------------
# DRP2
# -------------------------------------------------------------------------------------------------

file(GLOB DRP2_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_drp2 OBJECT ${DRP2_SRC})

target_include_directories(datoviz_drp2
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/external
)

target_compile_definitions(datoviz_drp2 PUBLIC ${DVZ_COMPILE_DEFINITIONS})
if(TARGET datoviz_vklite)
    target_link_libraries(datoviz_drp2 PUBLIC datoviz_vklite)
    target_compile_definitions(datoviz_drp2 PUBLIC DVZ_DRP2_HAS_VKLITE=1)

    if(APPLE)
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
            set(_dvz_shaderc_dir "${PROJECT_SOURCE_DIR}/libs/shaderc/macos_arm64")
        else()
            set(_dvz_shaderc_dir "${PROJECT_SOURCE_DIR}/libs/shaderc/macos_x86_64")
        endif()
        set(_dvz_shaderc_lib "${_dvz_shaderc_dir}/libshaderc_shared.dylib")
    elseif(WIN32)
        set(_dvz_shaderc_dir "${PROJECT_SOURCE_DIR}/libs/shaderc/windows")
        set(_dvz_shaderc_lib "${_dvz_shaderc_dir}/libshaderc_shared.dll")
    else()
        set(_dvz_shaderc_dir "${PROJECT_SOURCE_DIR}/libs/shaderc/linux")
        set(_dvz_shaderc_lib "${_dvz_shaderc_dir}/libshaderc_shared.so")
    endif()

    if(EXISTS "${_dvz_shaderc_lib}")
        if(NOT TARGET datoviz_shaderc)
            add_library(datoviz_shaderc SHARED IMPORTED GLOBAL)
            set_target_properties(datoviz_shaderc PROPERTIES IMPORTED_LOCATION "${_dvz_shaderc_lib}")
        endif()
        target_link_libraries(datoviz_drp2 PUBLIC datoviz_shaderc)
    else()
        message(FATAL_ERROR "shaderc shared library not found: ${_dvz_shaderc_lib}")
    endif()
else()
    target_compile_definitions(datoviz_drp2 PUBLIC DVZ_DRP2_HAS_VKLITE=0)
endif()
if(WIN32)
    target_compile_definitions(datoviz_drp2 PRIVATE DVZ_SHARED)
endif()
