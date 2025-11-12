# -------------------------------------------------------------------------------------------------
# Video library
# -------------------------------------------------------------------------------------------------

file(GLOB VIDEO_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")

set(KVZ_IMPL "${CMAKE_CURRENT_SOURCE_DIR}/encoder_backend_kvazaar.c")
set(KVZ_STUB "${CMAKE_CURRENT_SOURCE_DIR}/encoder_backend_kvazaar_stub.c")

if(DVZ_HAS_KVZ)
    list(REMOVE_ITEM VIDEO_SRC ${KVZ_STUB})
else()
    list(REMOVE_ITEM VIDEO_SRC ${KVZ_IMPL})
endif()

if(NOT DVZ_HAS_CUDA)
    list(FILTER VIDEO_SRC EXCLUDE REGEX ".*/encoder_backend_nvenc\\.c.*")
endif()
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

if(DVZ_HAS_KVZ)
    target_include_directories(datoviz_video
        PUBLIC
        ${PROJECT_SOURCE_DIR}/external/kvazaar/src
    )
    target_link_libraries(datoviz_video PUBLIC kvazaar::kvazaar)
endif()

if(WIN32)
    target_compile_definitions(datoviz_video PRIVATE DVZ_SHARED)
endif()

if(DVZ_HAS_CUDA)
    if(NOT CUDAToolkit_FOUND)
        message(FATAL_ERROR "DVZ_HAS_CUDA is ON but CUDAToolkit was not propagated to src/video.")
    endif()

    target_include_directories(datoviz_video
        PUBLIC
            ${CUDAToolkit_INCLUDE_DIRS}
    )
    target_link_libraries(datoviz_video PUBLIC CUDA::cuda_driver)

    if(DVZ_NVENC_LIB_DIR)
        target_link_directories(datoviz_video PRIVATE "${DVZ_NVENC_LIB_DIR}")
        set_property(TARGET datoviz_video APPEND PROPERTY BUILD_RPATH "${DVZ_NVENC_LIB_DIR}")
    endif()

    if(DVZ_NVENC_LIB)
        message(STATUS "Linking NVENC library: ${DVZ_NVENC_LIB}")
        target_link_libraries(datoviz_video PUBLIC ${DVZ_NVENC_LIB})
    else()
        message(FATAL_ERROR "DVZ_HAS_CUDA is ON but DVZ_NVENC_LIB is unset.")
    endif()
endif()
