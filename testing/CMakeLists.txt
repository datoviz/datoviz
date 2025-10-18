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
