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
    testing
)

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

# Register it as a CTest entry
add_test(NAME dvztest COMMAND dvztest)
