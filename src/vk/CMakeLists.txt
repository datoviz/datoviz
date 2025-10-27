# -------------------------------------------------------------------------------------------------
# Vulkan library
# -------------------------------------------------------------------------------------------------

file(GLOB VK_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_vk OBJECT
    ${VK_SRC}
    "${CMAKE_CURRENT_SOURCE_DIR}/../../external/vk_mem_alloc.cpp"
)

target_include_directories(datoviz_vk
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/common
        ${PROJECT_SOURCE_DIR}/external
        ${PROJECT_SOURCE_DIR}/external/volk
)

target_compile_definitions(datoviz_vk
    PUBLIC
        ${DVZ_COMPILE_DEFINITIONS}
        VK_NO_PROTOTYPES
        VMA_STATIC_VULKAN_FUNCTIONS=0
        VMA_DYNAMIC_VULKAN_FUNCTIONS=0
)

if(WIN32)
    target_compile_definitions(datoviz_vk PRIVATE DVZ_SHARED)
endif()
