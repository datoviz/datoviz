# -------------------------------------------------------------------------------------------------
# Common library
# -------------------------------------------------------------------------------------------------

# Common module (OBJECT library)
file(GLOB COMMON_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_common OBJECT ${COMMON_SRC})

target_include_directories(datoviz_common
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/external/mimalloc/include
    INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR} # so other modules see _*.h
)

# Propagate global compile definitions (OS/Compiler flags)
target_compile_definitions(datoviz_common PUBLIC ${COMPILE_DEFINITIONS})
if(WIN32)
    # Export symbols when building the shared library on Windows
    target_compile_definitions(datoviz_common PRIVATE DVZ_SHARED)
endif()

if(DVZ_THREADING_TARGET)
    target_link_libraries(datoviz_common PUBLIC ${DVZ_THREADING_TARGET})
endif()
