# -------------------------------------------------------------------------------------------------
# Optional Qt bridge provider
# -------------------------------------------------------------------------------------------------

string(TOUPPER "${DVZ_ENABLE_QT_BRIDGE}" _dvz_qt_bridge_mode)
if(NOT _dvz_qt_bridge_mode IN_LIST _dvz_qt_bridge_modes)
    message(FATAL_ERROR "DVZ_ENABLE_QT_BRIDGE must be AUTO, ON, or OFF.")
endif()

if(_dvz_qt_bridge_mode STREQUAL "OFF")
    message(STATUS "DVZ_ENABLE_QT_BRIDGE is OFF - skipping datoviz_qtbridge")
    return()
endif()

include("${PROJECT_SOURCE_DIR}/cmake/dvz_qt.cmake")
dvz_find_qt6_gui(_dvz_qt_target)

if(NOT _dvz_qt_target)
    if(_dvz_qt_bridge_mode STREQUAL "ON")
        message(FATAL_ERROR "DVZ_ENABLE_QT_BRIDGE is ON but Qt6 Gui development files were not found.")
    endif()
    message(STATUS "Qt6 Gui not found - skipping datoviz_qtbridge")
    return()
endif()

add_library(datoviz_qtbridge SHARED
    qt_bridge.cpp
)

target_link_libraries(datoviz_qtbridge
    PRIVATE
    ${_dvz_qt_target}
)

target_include_directories(datoviz_qtbridge
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(datoviz_qtbridge PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME "datoviz_qtbridge"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qtbridge"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qtbridge"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qtbridge"
)

if(APPLE)
    set_target_properties(datoviz_qtbridge PROPERTIES
        BUILD_RPATH  "@loader_path"
        INSTALL_RPATH "@loader_path"
    )
elseif(UNIX)
    set_target_properties(datoviz_qtbridge PROPERTIES
        BUILD_RPATH  "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()

if(DVZ_INSTALL)
    install(TARGETS datoviz_qtbridge
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
    )
endif()

set(DVZ_HAS_QT_BRIDGE 1 CACHE INTERNAL "Qt bridge provider availability flag" FORCE)
set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS datoviz_qtbridge)
