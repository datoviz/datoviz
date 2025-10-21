# -------------------------------------------------------------------------------------------------
# Math library
# -------------------------------------------------------------------------------------------------

file(GLOB DS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_ds OBJECT ${DS_SRC})

target_include_directories(datoviz_ds
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
)

target_compile_definitions(datoviz_ds PUBLIC ${COMPILE_DEFINITIONS})
if(WIN32)
    target_compile_definitions(datoviz_ds PRIVATE DVZ_SHARED)
endif()

if(";${DVZ_ACTIVE_SANITIZERS};" MATCHES ";memory;")
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/map.cpp
        PROPERTIES COMPILE_OPTIONS "-fno-sanitize=memory;-fno-sanitize-memory-track-origins")
endif()
