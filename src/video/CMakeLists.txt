# -------------------------------------------------------------------------------------------------
# Video library
# -------------------------------------------------------------------------------------------------

set(VIDEO_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder_core.c
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder_backend.c
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder_backend_stub.c
    ${CMAKE_CURRENT_SOURCE_DIR}/encoder_mux_mp4.c
    ${CMAKE_CURRENT_SOURCE_DIR}/video_sink.c
)

if(DVZ_HAS_KVZ)
    list(APPEND VIDEO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/encoder_backend_kvazaar.c)
endif()

if(DVZ_HAS_NVENC)
    list(APPEND VIDEO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/encoder_backend_nvenc.c)
endif()

add_library(datoviz_video OBJECT ${VIDEO_SRC})

target_include_directories(datoviz_video
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/src/thread
)

target_include_directories(datoviz_video
    SYSTEM PUBLIC
        ${PROJECT_SOURCE_DIR}/external
        ${PROJECT_SOURCE_DIR}/external/volk
)

target_compile_definitions(datoviz_video
    PUBLIC
    ${DVZ_COMPILE_DEFINITIONS}
    VK_NO_PROTOTYPES
)

if(DVZ_HAS_KVZ)
    target_link_libraries(datoviz_video PUBLIC ${DVZ_KVAZAAR_TARGET})
endif()

if(WIN32)
    target_compile_definitions(datoviz_video PRIVATE DVZ_SHARED)
endif()

if(DVZ_HAS_NVENC)
    if(NOT CUDAToolkit_FOUND)
        message(FATAL_ERROR "DVZ_HAS_NVENC is ON but CUDAToolkit was not propagated to src/video.")
    endif()

    target_include_directories(datoviz_video
        PUBLIC
            ${CUDAToolkit_INCLUDE_DIRS}
    )
    # libcuda and libnvidia-encode are loaded lazily via dlopen at runtime — no link-time dep.
    if(NOT WIN32)
        target_link_libraries(datoviz_video PRIVATE dl)
    endif()

    if(DVZ_NVENC_LIB_DIR)
        # Keep rpath so the runtime loader can find the library in non-standard locations.
        set_property(TARGET datoviz_video APPEND PROPERTY BUILD_RPATH "${DVZ_NVENC_LIB_DIR}")
    endif()

    if(DVZ_NVENC_LIB)
        message(STATUS "NVENC library found at: ${DVZ_NVENC_LIB} (loaded lazily at runtime)")
    else()
        message(FATAL_ERROR "DVZ_HAS_NVENC is ON but DVZ_NVENC_LIB is unset.")
    endif()
endif()
