# -------------------------------------------------------------------------------------------------
# DRP2
# -------------------------------------------------------------------------------------------------

set(DRP2_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/command_metadata.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/packet.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/recording.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/semantic.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/serialization.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/stream.c"
)

if(TARGET datoviz_vklite)
    list(APPEND DRP2_SRC
        "${CMAKE_CURRENT_SOURCE_DIR}/backend.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/objects.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/pass.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/pipeline.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/transfer.c"
    )
endif()

add_library(datoviz_drp2 OBJECT ${DRP2_SRC})

target_include_directories(datoviz_drp2
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/external
    ${PROJECT_SOURCE_DIR}/external/volk
)

target_compile_definitions(datoviz_drp2 PUBLIC ${DVZ_COMPILE_DEFINITIONS})
target_link_libraries(datoviz_drp2 PUBLIC datoviz_shader)
if(TARGET datoviz_vklite)
    target_link_libraries(datoviz_drp2 PUBLIC datoviz_vklite)
    target_compile_definitions(datoviz_drp2 PUBLIC DVZ_DRP2_HAS_VKLITE=1)
else()
    target_compile_definitions(datoviz_drp2 PUBLIC DVZ_DRP2_HAS_VKLITE=0)
endif()
if(WIN32)
    target_compile_definitions(datoviz_drp2 PRIVATE DVZ_SHARED)
endif()
