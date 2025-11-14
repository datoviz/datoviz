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
    find_package(glfw3 3.3 QUIET)
    if(glfw3_FOUND)
        set(_dvz_glfw_enabled 1)
        target_include_directories(datoviz_window PUBLIC ${glfw3_INCLUDE_DIRS})
        target_link_libraries(datoviz_window PUBLIC glfw)
    else()
        message(WARNING "DVZ_WITH_GLFW is ON but glfw3 was not found; disabling GLFW backend.")
    endif()
endif()
target_compile_definitions(datoviz_window PUBLIC DVZ_WITH_GLFW=${_dvz_glfw_enabled})
set(DVZ_WINDOW_GLFW_ENABLED ${_dvz_glfw_enabled} CACHE INTERNAL "datoviz GLFW backend enabled" FORCE)
