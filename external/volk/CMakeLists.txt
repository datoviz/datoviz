file(GLOB VOLK_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
add_library(datoviz_volk STATIC ${VOLK_SRC})

set_target_properties(datoviz_volk PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(datoviz_volk
    PUBLIC
        ${PROJECT_SOURCE_DIR}/external/volk
)

target_compile_definitions(datoviz_volk PUBLIC VOLK_STATIC_DEFINES)
