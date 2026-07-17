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
if(TARGET datoviz_vklite)
    target_link_libraries(datoviz_drp2 PUBLIC datoviz_vklite)
    target_compile_definitions(datoviz_drp2 PUBLIC DVZ_DRP2_HAS_VKLITE=1)
    target_compile_definitions(datoviz_drp2 PUBLIC DVZ_HAS_SHADERC=${DVZ_HAS_SHADERC})
    if(DVZ_HAS_SHADERC)
        target_compile_definitions(datoviz_drp2 PRIVATE DVZ_SHADERC_STATIC=${DVZ_SHADERC_STATIC})
        if(DVZ_SHADERC_STATIC)
            target_link_libraries(datoviz_drp2 PUBLIC ${DVZ_SHADERC_TARGET})
        else()
            target_include_directories(datoviz_drp2 PRIVATE "${DVZ_SHADERC_INCLUDE_DIR}")
            target_compile_definitions(datoviz_drp2 PRIVATE
                DVZ_SHADERC_LIB_PATH="${DVZ_SHADERC_RUNTIME_LIBRARY}")
        endif()
    endif()
    if(NOT WIN32)
        target_link_libraries(datoviz_drp2 PRIVATE dl)
    endif()
else()
    target_compile_definitions(datoviz_drp2 PUBLIC DVZ_DRP2_HAS_VKLITE=0)
endif()
if(WIN32)
    target_compile_definitions(datoviz_drp2 PRIVATE DVZ_SHARED)
endif()
