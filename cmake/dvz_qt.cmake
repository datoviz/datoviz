# -------------------------------------------------------------------------------------------------
# Optional Qt discovery helpers
# -------------------------------------------------------------------------------------------------

function(dvz_find_qt6_gui out_target)
    find_package(Qt6 QUIET COMPONENTS Gui)

    if(TARGET Qt6::Gui)
        set(${out_target} Qt6::Gui PARENT_SCOPE)
        return()
    endif()

    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(DVZ_QT6_GUI QUIET IMPORTED_TARGET Qt6Gui Qt6Core)
        if(TARGET PkgConfig::DVZ_QT6_GUI)
            set(${out_target} PkgConfig::DVZ_QT6_GUI PARENT_SCOPE)
            return()
        endif()
    endif()

    set(${out_target} "" PARENT_SCOPE)
endfunction()


function(dvz_find_qt6_widgets out_target)
    find_package(Qt6 QUIET COMPONENTS Widgets)

    if(TARGET Qt6::Widgets)
        set(${out_target} Qt6::Widgets PARENT_SCOPE)
        return()
    endif()

    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(DVZ_QT6_WIDGETS QUIET IMPORTED_TARGET Qt6Widgets Qt6Gui Qt6Core)
        if(TARGET PkgConfig::DVZ_QT6_WIDGETS)
            set(${out_target} PkgConfig::DVZ_QT6_WIDGETS PARENT_SCOPE)
            return()
        endif()
    endif()

    set(${out_target} "" PARENT_SCOPE)
endfunction()
