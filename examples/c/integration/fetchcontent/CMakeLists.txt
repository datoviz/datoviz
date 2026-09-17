cmake_minimum_required(VERSION 3.21)
project(datoviz_fetchcontent_example C)

include(FetchContent)

set(DATOVIZ_FETCHCONTENT_SOURCE_DIR "" CACHE PATH "Optional local Datoviz source checkout")
set(DATOVIZ_FETCHCONTENT_TAG "main" CACHE STRING "Datoviz Git tag or ref for FetchContent")
set(
    DATOVIZ_FETCHCONTENT_PRELOAD_GLFW "OFF" CACHE STRING
    "Parent-owned GLFW target to test before embedding Datoviz: OFF, PLAIN, or NAMESPACED")
set_property(
    CACHE DATOVIZ_FETCHCONTENT_PRELOAD_GLFW PROPERTY STRINGS OFF PLAIN NAMESPACED)
option(
    DATOVIZ_FETCHCONTENT_PRELOAD_GLSLC_VARIABLES
    "Define parent-owned generic GLSLC variables before embedding Datoviz"
    OFF)

if(DATOVIZ_FETCHCONTENT_PRELOAD_GLSLC_VARIABLES)
    set(GLSLC "parent-owned-glslc" CACHE STRING "Parent-owned shader compiler setting")
    set(GLSLC_PATH "parent-owned-glslc-path" CACHE STRING "Parent-owned shader search setting")
    set(_parent_glslc "${GLSLC}")
    set(_parent_glslc_path "${GLSLC_PATH}")
endif()

if(NOT DATOVIZ_FETCHCONTENT_PRELOAD_GLFW STREQUAL "OFF")
    if(NOT DATOVIZ_FETCHCONTENT_PRELOAD_GLFW MATCHES "^(PLAIN|NAMESPACED)$")
        message(FATAL_ERROR
            "DATOVIZ_FETCHCONTENT_PRELOAD_GLFW must be OFF, PLAIN, or NAMESPACED")
    endif()
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

    if(DATOVIZ_FETCHCONTENT_PRELOAD_GLFW STREQUAL "NAMESPACED")
        add_library(glfw::glfw ALIAS glfw)
    endif()
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

if(DATOVIZ_FETCHCONTENT_PRELOAD_GLSLC_VARIABLES)
    if(NOT "${GLSLC}" STREQUAL "${_parent_glslc}"
            OR NOT "${GLSLC_PATH}" STREQUAL "${_parent_glslc_path}")
        message(FATAL_ERROR "Datoviz modified parent-owned GLSLC configuration variables")
    endif()
endif()

add_executable(datoviz_fetchcontent_example main.c)
target_link_libraries(datoviz_fetchcontent_example PRIVATE datoviz::datoviz)
