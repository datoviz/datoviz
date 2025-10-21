# -------------------------------------------------------------------------------------------------
# Generic testing CMake file
# -------------------------------------------------------------------------------------------------

add_library(testing STATIC testing.cpp)

target_include_directories(testing
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/common
)

# Allow linking from C
set_target_properties(testing PROPERTIES
    LINKER_LANGUAGE CXX
)

# -------------------------------------------------------------------------------------------------
# Datoviz test runner
# -------------------------------------------------------------------------------------------------

# All C source files inside tests/
file(GLOB TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/*/tests/*.c*"
    "dvztest.c"
)

# Build one executable: dvztest
add_executable(dvztest ${TEST_SOURCES})

# Link to the common module and the shared testing library
target_link_libraries(dvztest
    PRIVATE
    datoviz_common
    datoviz_ds
    datoviz_fileio
    datoviz_math
    datoviz_thread
    testing
)

if(DVZ_WITH_MIMALLOC)
    target_link_libraries(dvztest PRIVATE $<$<CONFIG:Release>:mimalloc-static>)
endif()

# Track testing targets so top-level logic can apply shared instrumentation (coverage, sanitizers, ...).
foreach(_dvz_test_target testing dvztest)
    if(TARGET ${_dvz_test_target})
        set_property(GLOBAL APPEND PROPERTY DVZ_ALL_TARGETS ${_dvz_test_target})
    endif()
endforeach()

# Include test headers and internal paths
target_include_directories(dvztest
    PRIVATE

    # ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${PROJECT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/common
)

# Ensure OS/Compiler macros are available to tests
target_compile_definitions(dvztest PRIVATE ${COMPILE_DEFINITIONS})

# -------------------------------------------------------------------------------------------------
# Public header probe
# -------------------------------------------------------------------------------------------------
add_executable(dvz_public_header_probe dvz_public_header_probe.c)

target_link_libraries(dvz_public_header_probe
    PRIVATE
    datoviz
)

target_include_directories(dvz_public_header_probe
    PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(dvz_public_header_probe PRIVATE ${COMPILE_DEFINITIONS})

add_custom_command(TARGET dvz_public_header_probe
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E env
    "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:datoviz>:$ENV{LD_LIBRARY_PATH}"
    "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:datoviz>:$ENV{DYLD_LIBRARY_PATH}"
    "PATH=$<TARGET_FILE_DIR:datoviz>:$ENV{PATH}"
    $<TARGET_FILE:dvz_public_header_probe>
    WORKING_DIRECTORY $<TARGET_FILE_DIR:dvz_public_header_probe>
)

if(DVZ_ENABLE_ASAN_IN_DEBUG AND DVZ_SANITIZER_FLAGS)
    foreach(_target testing dvztest)
        if(TARGET ${_target})
            target_compile_options(${_target} PRIVATE $<$<CONFIG:Debug>:${DVZ_SANITIZER_FLAGS}>)
            target_link_options(${_target} PRIVATE $<$<CONFIG:Debug>:${DVZ_SANITIZER_FLAGS}>)
        endif()
    endforeach()
endif()

# Register it as a CTest entry
add_test(NAME dvztest COMMAND dvztest)
