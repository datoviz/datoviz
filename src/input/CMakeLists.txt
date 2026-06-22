file(GLOB INPUT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_input OBJECT ${INPUT_SRC})

target_include_directories(datoviz_input
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/common
)

target_compile_definitions(datoviz_input PUBLIC ${DVZ_COMPILE_DEFINITIONS})
if(WIN32)
    target_compile_definitions(datoviz_input PRIVATE DVZ_SHARED)
endif()
