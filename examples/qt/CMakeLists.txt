# -------------------------------------------------------------------------------------------------
# Qt examples - optional hosted toolkit adapters outside core
# -------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED PROJECT_SOURCE_DIR OR NOT TARGET datoviz OR NOT TARGET datoviz_app)
    message(FATAL_ERROR
        "examples/qt must be built as part of the datoviz source tree "
        "(add_subdirectory from the top-level CMakeLists.txt).")
endif()

find_package(Qt6 QUIET COMPONENTS Gui)
find_package(Qt6 QUIET COMPONENTS Widgets)

set(_dvz_qt_target "")
if(TARGET Qt6::Gui)
    set(_dvz_qt_target Qt6::Gui)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(DVZ_QT6 QUIET IMPORTED_TARGET Qt6Gui Qt6Core)
        if(TARGET PkgConfig::DVZ_QT6)
            set(_dvz_qt_target PkgConfig::DVZ_QT6)
        endif()
    endif()
endif()

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

set(_dvz_qt_widgets_target "")
if(TARGET Qt6::Widgets)
    set(_dvz_qt_widgets_target Qt6::Widgets)
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(DVZ_QT6_WIDGETS QUIET IMPORTED_TARGET Qt6Widgets Qt6Gui Qt6Core)
        if(TARGET PkgConfig::DVZ_QT6_WIDGETS)
            set(_dvz_qt_widgets_target PkgConfig::DVZ_QT6_WIDGETS)
        endif()
    endif()
endif()

if(_dvz_qt_widgets_target)
    add_executable(hosted_qt_widgets hosted_qt_widgets.cpp hosted_qt_adapter.cpp)

    target_link_libraries(hosted_qt_widgets
        PRIVATE
        datoviz
        ${_dvz_qt_widgets_target}
    )

    target_include_directories(hosted_qt_widgets
        PRIVATE
        ${PROJECT_SOURCE_DIR}/include
    )

    set_target_properties(hosted_qt_widgets PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        AUTOMOC OFF
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/examples/qt"
    )

    if(APPLE)
        set_target_properties(hosted_qt_widgets PROPERTIES
            BUILD_RPATH  "@executable_path;@loader_path"
            INSTALL_RPATH "@executable_path;@loader_path"
        )
    endif()

    set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS hosted_qt_widgets)
else()
    message(STATUS "Qt6 Widgets not found - skipping hosted_qt_widgets")
endif()
