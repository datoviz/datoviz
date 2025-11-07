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
