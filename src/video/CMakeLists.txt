# -------------------------------------------------------------------------------------------------
# Video library
# -------------------------------------------------------------------------------------------------

file(GLOB VIDEO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_video OBJECT
    ${VIDEO_SRC}
)

target_include_directories(datoviz_video
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/volk
)

target_compile_definitions(datoviz_video
    PUBLIC
    ${DVZ_COMPILE_DEFINITIONS}
    VK_NO_PROTOTYPES
)

if(WIN32)
    target_compile_definitions(datoviz_video PRIVATE DVZ_SHARED)
endif()

if(DVZ_ENABLE_CUDA AND CUDAToolkit_FOUND)
    target_include_directories(datoviz_video
        PUBLIC
            ${CUDAToolkit_INCLUDE_DIRS}
    )
    target_link_libraries(datoviz_video PUBLIC CUDA::cuda_driver)

    if(WIN32)
        set(_dvz_nvenc_lib_dir "${PROJECT_SOURCE_DIR}/libs/nvenc/windows")
    elseif(APPLE)
        set(_dvz_nvenc_lib_dir "${PROJECT_SOURCE_DIR}/libs/nvenc/macos")
    else()
        set(_dvz_nvenc_lib_dir "${PROJECT_SOURCE_DIR}/libs/nvenc/linux")
    endif()

    if(EXISTS "${_dvz_nvenc_lib_dir}")
        target_link_directories(datoviz_video PRIVATE "${_dvz_nvenc_lib_dir}")
        set_property(TARGET datoviz_video APPEND PROPERTY BUILD_RPATH "${_dvz_nvenc_lib_dir}")
    endif()

    set(_dvz_nvenc_names nvencodeapi nvencodeapi64)
    if(UNIX AND NOT APPLE)
        list(APPEND _dvz_nvenc_names nvidia-encode)
    endif()

    find_library(_dvz_nvenc_lib
        NAMES ${_dvz_nvenc_names}
        HINTS ${_dvz_nvenc_lib_dir}
    )

    if(_dvz_nvenc_lib)
        message(STATUS "Linking NVENC library: ${_dvz_nvenc_lib}")
        target_link_libraries(datoviz_video PUBLIC ${_dvz_nvenc_lib})
    else()
        message(FATAL_ERROR
            "DVZ_ENABLE_CUDA is ON but no NVENC library was found (looked for: ${_dvz_nvenc_names}).")
    endif()
endif()
