# Common module (OBJECT library)
file(GLOB COMMON_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c")
add_library(datoviz_common OBJECT ${COMMON_SRC})

target_include_directories(datoviz_common
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Propagate global compile definitions (OS/Compiler flags)
target_compile_definitions(datoviz_common PUBLIC ${COMPILE_DEFINITIONS})

# --- Single test executable for the common module ---

# Optionally build tests for this module
option(DVZ_BUILD_TESTS "Build common module tests" ON)

if(DVZ_BUILD_TESTS)
    # All C source files inside tests/
    file(GLOB COMMON_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c")

    # Build one executable: test_common
    add_executable(test_common ${COMMON_TEST_SOURCES})

    # Link to the common module and the shared testing library
    target_link_libraries(test_common
        PRIVATE
            datoviz_common
            testing
    )

    # Include test headers and internal paths
    target_include_directories(test_common
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
            ${PROJECT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # Ensure OS/Compiler macros are available to tests
    target_compile_definitions(test_common PRIVATE ${COMPILE_DEFINITIONS})

    # Register it as a CTest entry
    add_test(NAME test_common COMMAND test_common)
endif()
