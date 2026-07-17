set(DVZ_KVAZAAR_SOURCE_RESOLVED "OFF")
set(DVZ_KVAZAAR_TARGET "")

if(DVZ_ENABLE_KVAZAAR AND NOT DVZ_KVAZAAR_SOURCE STREQUAL "OFF")
    set(_dvz_kvz_dir "${DVZ_EXTERNAL_DIR}/kvazaar")
    dvz_dependency_source_order(_dvz_kvz_sources "${DVZ_KVAZAAR_SOURCE}")

    foreach(_dvz_kvz_source IN LISTS _dvz_kvz_sources)
        if(DVZ_HAS_KVZ)
            break()
        endif()

        if(_dvz_kvz_source STREQUAL "SYSTEM")
            find_package(kvazaar CONFIG QUIET)

            if(TARGET kvazaar::kvazaar)
                set(DVZ_KVAZAAR_TARGET kvazaar::kvazaar)
                set(DVZ_HAS_KVZ 1)
                set(DVZ_KVAZAAR_SOURCE_RESOLVED "SYSTEM")
            elseif(TARGET kvazaar)
                if(NOT TARGET kvazaar::kvazaar)
                    add_library(kvazaar::kvazaar ALIAS kvazaar)
                endif()
                set(DVZ_KVAZAAR_TARGET kvazaar::kvazaar)
                set(DVZ_HAS_KVZ 1)
                set(DVZ_KVAZAAR_SOURCE_RESOLVED "SYSTEM")
            endif()

            if(NOT DVZ_HAS_KVZ)
                find_package(PkgConfig QUIET)

                if(PkgConfig_FOUND)
                    pkg_check_modules(KVAZAAR QUIET IMPORTED_TARGET kvazaar)
                endif()

                if(TARGET PkgConfig::KVAZAAR)
                    if(NOT TARGET kvazaar::kvazaar)
                        add_library(kvazaar::kvazaar INTERFACE IMPORTED)
                        set_target_properties(
                            kvazaar::kvazaar PROPERTIES
                            INTERFACE_LINK_LIBRARIES PkgConfig::KVAZAAR)
                    endif()
                    set(DVZ_KVAZAAR_TARGET kvazaar::kvazaar)
                    set(DVZ_HAS_KVZ 1)
                    set(DVZ_KVAZAAR_SOURCE_RESOLVED "SYSTEM")
                endif()
            endif()
        elseif(_dvz_kvz_source STREQUAL "VENDORED")
            if(EXISTS "${_dvz_kvz_dir}/CMakeLists.txt")
                if(DEFINED BUILD_SHARED_LIBS)
                    set(_dvz_prev_build_shared_libs "${BUILD_SHARED_LIBS}")
                    set(_dvz_prev_build_shared_libs_defined 1)
                else()
                    set(_dvz_prev_build_shared_libs "")
                    set(_dvz_prev_build_shared_libs_defined 0)
                endif()

                if(DEFINED BUILD_TESTS)
                    set(_dvz_prev_build_tests "${BUILD_TESTS}")
                    set(_dvz_prev_build_tests_defined 1)
                else()
                    set(_dvz_prev_build_tests "")
                    set(_dvz_prev_build_tests_defined 0)
                endif()

                if(DEFINED GIT_SUBMODULE)
                    set(_dvz_prev_git_submodule "${GIT_SUBMODULE}")
                    set(_dvz_prev_git_submodule_defined 1)
                else()
                    set(_dvz_prev_git_submodule "")
                    set(_dvz_prev_git_submodule_defined 0)
                endif()

                set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
                set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
                set(BUILD_KVAZAAR_BINARY OFF CACHE BOOL "" FORCE)
                set(GIT_SUBMODULE OFF CACHE BOOL "" FORCE)
                set(USE_CRYPTO OFF CACHE BOOL "" FORCE)

                add_subdirectory(
                    "${_dvz_kvz_dir}"
                    "${CMAKE_CURRENT_BINARY_DIR}/external/kvazaar"
                    EXCLUDE_FROM_ALL)

                if(TARGET kvazaar)
                    if(NOT TARGET kvazaar::kvazaar)
                        add_library(kvazaar::kvazaar ALIAS kvazaar)
                    endif()
                    set_target_properties(kvazaar PROPERTIES POSITION_INDEPENDENT_CODE ON)
                    if(MSVC AND TARGET PThreads4W::PThreads4W)
                        # Datoviz already selected the pthread implementation for MSVC. Reuse that
                        # target for Kvazaar so its vendored pthread shim cannot shadow PThreads4W.
                        set(
                            _dvz_kvz_threadwrapper_include
                            "${_dvz_kvz_dir}/src/threadwrapper/include")
                        foreach(
                            _dvz_kvz_include_prop
                            IN ITEMS INCLUDE_DIRECTORIES INTERFACE_INCLUDE_DIRECTORIES)
                            get_target_property(
                                _dvz_kvz_include_dirs kvazaar ${_dvz_kvz_include_prop})
                            if(_dvz_kvz_include_dirs)
                                list(
                                    REMOVE_ITEM _dvz_kvz_include_dirs
                                    "${_dvz_kvz_threadwrapper_include}")
                                set_property(
                                    TARGET kvazaar PROPERTY ${_dvz_kvz_include_prop}
                                    "${_dvz_kvz_include_dirs}")
                            endif()
                        endforeach()

                        set_source_files_properties(
                            "${_dvz_kvz_dir}/src/threadwrapper/src/pthread.cpp"
                            "${_dvz_kvz_dir}/src/threadwrapper/src/semaphore.cpp"
                            DIRECTORY "${_dvz_kvz_dir}"
                            PROPERTIES HEADER_FILE_ONLY ON)
                        target_link_libraries(kvazaar PUBLIC PThreads4W::PThreads4W)

                        unset(_dvz_kvz_include_dirs)
                        unset(_dvz_kvz_include_prop)
                        unset(_dvz_kvz_threadwrapper_include)
                    endif()

                    get_target_property(_dvz_kvazaar_type kvazaar TYPE)
                    if(_dvz_kvazaar_type STREQUAL "STATIC_LIBRARY")
                        target_compile_definitions(kvazaar INTERFACE KVZ_STATIC_LIB)
                    endif()
                    unset(_dvz_kvazaar_type)
                    set(DVZ_KVAZAAR_TARGET kvazaar::kvazaar)
                    set(DVZ_HAS_KVZ 1)
                    set(DVZ_KVAZAAR_SOURCE_RESOLVED "VENDORED")
                endif()

                if(_dvz_prev_build_shared_libs_defined)
                    set(BUILD_SHARED_LIBS "${_dvz_prev_build_shared_libs}" CACHE BOOL "" FORCE)
                else()
                    unset(BUILD_SHARED_LIBS CACHE)
                endif()

                if(_dvz_prev_build_tests_defined)
                    set(BUILD_TESTS "${_dvz_prev_build_tests}" CACHE BOOL "" FORCE)
                else()
                    unset(BUILD_TESTS CACHE)
                endif()

                if(_dvz_prev_git_submodule_defined)
                    set(GIT_SUBMODULE "${_dvz_prev_git_submodule}" CACHE BOOL "" FORCE)
                else()
                    unset(GIT_SUBMODULE CACHE)
                endif()

                unset(BUILD_KVAZAAR_BINARY CACHE)
                unset(USE_CRYPTO CACHE)

                unset(_dvz_prev_build_shared_libs)
                unset(_dvz_prev_build_shared_libs_defined)
                unset(_dvz_prev_build_tests)
                unset(_dvz_prev_build_tests_defined)
                unset(_dvz_prev_git_submodule)
                unset(_dvz_prev_git_submodule_defined)
            endif()
        endif()
    endforeach()
endif()

if(DVZ_ENABLE_KVAZAAR AND NOT DVZ_HAS_KVZ)
    if(DVZ_KVAZAAR_SOURCE STREQUAL "SYSTEM")
        message(FATAL_ERROR "DVZ_KVAZAAR_SOURCE=SYSTEM but system Kvazaar was not found.")
    elseif(DVZ_KVAZAAR_SOURCE STREQUAL "VENDORED")
        message(FATAL_ERROR "DVZ_KVAZAAR_SOURCE=VENDORED but external/kvazaar is missing.")
    elseif(DVZ_KVAZAAR_SOURCE STREQUAL "AUTO")
        message(WARNING "Kvazaar was not found from system packages or external/kvazaar; Kvazaar backend disabled.")
    endif()
endif()

set(DVZ_HAS_KVZ "${DVZ_HAS_KVZ}" CACHE INTERNAL "Kvazaar availability flag" FORCE)
set(DVZ_KVAZAAR_TARGET "${DVZ_KVAZAAR_TARGET}" CACHE INTERNAL "Resolved Kvazaar link target" FORCE)
set(DVZ_KVAZAAR_SOURCE_RESOLVED "${DVZ_KVAZAAR_SOURCE_RESOLVED}" CACHE INTERNAL "Resolved Kvazaar source" FORCE)
