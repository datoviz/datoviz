# -------------------------------------------------------------------------------------------------
# vklite library
# -------------------------------------------------------------------------------------------------

file(GLOB VKLITE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_vklite OBJECT
    ${VKLITE_SRC}
)

target_include_directories(datoviz_vklite
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/common
        # ${PROJECT_SOURCE_DIR}/external
        # ${PROJECT_SOURCE_DIR}/external/volk
)

target_compile_definitions(datoviz_vklite
    PUBLIC
        ${DVZ_COMPILE_DEFINITIONS}
        VK_NO_PROTOTYPES
        # VMA_STATIC_VULKAN_FUNCTIONS=0
        # VMA_DYNAMIC_VULKAN_FUNCTIONS=1
)

if(WIN32)
    target_compile_definitions(datoviz_vklite PRIVATE DVZ_SHARED)
endif()
