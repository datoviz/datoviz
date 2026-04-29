# -------------------------------------------------------------------------------------------------
# DRP2
# -------------------------------------------------------------------------------------------------

file(GLOB DRP2_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_drp2 OBJECT ${DRP2_SRC})

target_include_directories(datoviz_drp2
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
)

target_compile_definitions(datoviz_drp2 PUBLIC ${DVZ_COMPILE_DEFINITIONS})
if(TARGET datoviz_vklite)
    target_link_libraries(datoviz_drp2 PUBLIC datoviz_vklite)
    target_compile_definitions(datoviz_drp2 PUBLIC DVZ_DRP2_HAS_VKLITE=1)
else()
    target_compile_definitions(datoviz_drp2 PUBLIC DVZ_DRP2_HAS_VKLITE=0)
endif()
if(WIN32)
    target_compile_definitions(datoviz_drp2 PRIVATE DVZ_SHARED)
endif()
