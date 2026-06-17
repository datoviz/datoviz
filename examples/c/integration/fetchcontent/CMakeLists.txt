cmake_minimum_required(VERSION 3.21)
project(datoviz_fetchcontent_example C)

include(FetchContent)

set(DATOVIZ_FETCHCONTENT_SOURCE_DIR "" CACHE PATH "Optional local Datoviz source checkout")
set(DATOVIZ_FETCHCONTENT_TAG "v0.4.0" CACHE STRING "Datoviz Git tag or ref for FetchContent")

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
