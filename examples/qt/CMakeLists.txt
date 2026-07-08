# -------------------------------------------------------------------------------------------------
# Qt examples - optional hosted toolkit adapters outside core
# -------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED PROJECT_SOURCE_DIR OR NOT TARGET datoviz OR NOT TARGET datoviz_app)
    message(FATAL_ERROR
        "examples/qt must be built as part of the datoviz source tree "
        "(add_subdirectory from the top-level CMakeLists.txt).")
endif()

include("${PROJECT_SOURCE_DIR}/cmake/dvz_qt.cmake")
dvz_find_qt6_gui(_dvz_qt_target)

if(NOT _dvz_qt_target)
    message(STATUS "Qt6 Gui not found - skipping examples/qt")
    return()
endif()

add_executable(hosted_qt_smoke hosted_qt_smoke.cpp hosted_qt_adapter.cpp)

target_link_libraries(hosted_qt_smoke
    PRIVATE
    datoviz
    ${_dvz_qt_target}
)

target_include_directories(hosted_qt_smoke
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

set_target_properties(hosted_qt_smoke PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    AUTOMOC OFF
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/qt"
)

if(APPLE)
    set_target_properties(hosted_qt_smoke PROPERTIES
        BUILD_RPATH  "@executable_path;@loader_path"
        INSTALL_RPATH "@executable_path;@loader_path"
    )
endif()

set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS hosted_qt_smoke)

dvz_find_qt6_widgets(_dvz_qt_widgets_target)

if(_dvz_qt_widgets_target)
    add_executable(qt_hosting qt_hosting.cpp hosted_qt_adapter.cpp)

    target_link_libraries(qt_hosting
        PRIVATE
        datoviz
        ${_dvz_qt_widgets_target}
    )

    target_include_directories(qt_hosting
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
    )

    set_target_properties(qt_hosting PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        AUTOMOC OFF
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/qt"
    )

    if(APPLE)
        set_target_properties(qt_hosting PROPERTIES
            BUILD_RPATH  "@executable_path;@loader_path"
            INSTALL_RPATH "@executable_path;@loader_path"
        )
    endif()

    set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS qt_hosting)
else()
    message(STATUS "Qt6 Widgets not found - skipping qt_hosting")
endif()
