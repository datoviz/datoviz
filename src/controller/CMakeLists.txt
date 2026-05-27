# -------------------------------------------------------------------------------------------------
# Controller
# -------------------------------------------------------------------------------------------------

file(GLOB CONTROLLER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_controller OBJECT ${CONTROLLER_SRC})

target_include_directories(datoviz_controller
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/external/cglm/include
)

target_link_libraries(datoviz_controller
    PUBLIC
    datoviz_input
    datoviz_math
)

target_compile_definitions(datoviz_controller PUBLIC ${DVZ_COMPILE_DEFINITIONS})
if(WIN32)
    target_compile_definitions(datoviz_controller PRIVATE DVZ_SHARED)
endif()
