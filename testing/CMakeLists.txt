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

if(DVZ_ENABLE_ASAN_IN_DEBUG)
    if(DEFINED DVZ_SANITIZER_FLAGS AND NOT DVZ_SANITIZER_FLAGS STREQUAL "")
        set(_DVZ_SANITIZER_FLAGS "${DVZ_SANITIZER_FLAGS}")
    elseif(DEFINED DVZ_ASAN_FLAG AND NOT DVZ_ASAN_FLAG STREQUAL "")
        # Backward compatibility with older cached variables.
        set(_DVZ_SANITIZER_FLAGS "${DVZ_ASAN_FLAG}")
    elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
        set(_DVZ_SANITIZER_FLAGS "/fsanitize=address")
        message(WARNING "Only AddressSanitizer is available under MSVC; skipping other sanitizers for tests.")
    else()
        if(APPLE)
            set(_DVZ_SANITIZER_FLAGS "-fsanitize=address;-fsanitize=undefined")
            message(STATUS "Enabling AddressSanitizer and UndefinedBehaviorSanitizer for tests in Debug builds (LeakSanitizer is unavailable on macOS).")
        else()
            set(_DVZ_SANITIZER_FLAGS "-fsanitize=address;-fsanitize=undefined;-fsanitize=leak")
            message(STATUS "Enabling AddressSanitizer, LeakSanitizer, and UndefinedBehaviorSanitizer for tests in Debug builds.")
        endif()
    endif()
    if(_DVZ_SANITIZER_FLAGS)
        foreach(_target testing dvztest)
            if(TARGET ${_target})
                target_compile_options(${_target} PRIVATE $<$<CONFIG:Debug>:${_DVZ_SANITIZER_FLAGS}>)
                target_link_options(${_target} PRIVATE $<$<CONFIG:Debug>:${_DVZ_SANITIZER_FLAGS}>)
            endif()
        endforeach()
    endif()
endif()

# Register it as a CTest entry
add_test(NAME dvztest COMMAND dvztest)
