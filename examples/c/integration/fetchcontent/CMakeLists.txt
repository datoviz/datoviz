cmake_minimum_required(VERSION 3.21)
project(datoviz_fetchcontent_example C)

include(FetchContent)

set(DATOVIZ_FETCHCONTENT_SOURCE_DIR "" CACHE PATH "Optional local Datoviz source checkout")
set(DATOVIZ_FETCHCONTENT_TAG "v0.4-dev" CACHE STRING "Datoviz Git tag or ref for FetchContent")
option(
    DATOVIZ_FETCHCONTENT_PRELOAD_GLFW
    "Build GLFW in the parent project before embedding Datoviz"
    OFF)

if(DATOVIZ_FETCHCONTENT_PRELOAD_GLFW)
    if(NOT DATOVIZ_FETCHCONTENT_SOURCE_DIR)
        message(FATAL_ERROR
            "DATOVIZ_FETCHCONTENT_PRELOAD_GLFW requires DATOVIZ_FETCHCONTENT_SOURCE_DIR")
    endif()

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(
        "${DATOVIZ_FETCHCONTENT_SOURCE_DIR}/external/glfw"
        "${CMAKE_CURRENT_BINARY_DIR}/preexisting-glfw"
        EXCLUDE_FROM_ALL)
endif()

if(DATOVIZ_FETCHCONTENT_SOURCE_DIR)
    FetchContent_Declare(datoviz
        SOURCE_DIR "${DATOVIZ_FETCHCONTENT_SOURCE_DIR}"
    )
else()
    FetchContent_Declare(datoviz
        GIT_REPOSITORY https://github.com/datoviz/datoviz.git
        GIT_TAG "${DATOVIZ_FETCHCONTENT_TAG}"
    )
endif()

FetchContent_MakeAvailable(datoviz)

add_executable(datoviz_fetchcontent_example main.c)
target_link_libraries(datoviz_fetchcontent_example PRIVATE datoviz::datoviz)
