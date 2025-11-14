file(GLOB WINDOW_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_window OBJECT ${WINDOW_SRC})

target_include_directories(datoviz_window
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/src/common
)

target_compile_definitions(datoviz_window PUBLIC ${COMPILE_DEFINITIONS})

set(_dvz_glfw_enabled 0)
if(DVZ_WITH_GLFW)
    if(TARGET glfw)
        set(_dvz_glfw_enabled 1)
        target_link_libraries(datoviz_window PUBLIC glfw::glfw)
    else()
        message(WARNING "DVZ_WITH_GLFW is ON but GLFW target is missing; disabling GLFW backend.")
    endif()
endif()
target_compile_definitions(datoviz_window PUBLIC DVZ_WITH_GLFW=${_dvz_glfw_enabled})
set(DVZ_WINDOW_GLFW_ENABLED ${_dvz_glfw_enabled} CACHE INTERNAL "datoviz GLFW backend enabled" FORCE)
