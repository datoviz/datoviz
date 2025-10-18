
# -------------------------------------------------------------------------------------------------
# Detect the OS
# -------------------------------------------------------------------------------------------------
set(OS_LINUX 0)
set(OS_MACOS 0)
set(OS_WINDOWS 0)

if(UNIX AND NOT APPLE)
    set(OS_LINUX 1)
elseif(APPLE)
    set(OS_MACOS 1)
elseif(WIN32)
    set(OS_WINDOWS 1)
endif()

message("linux ${OS_LINUX}")

# -------------------------------------------------------------------------------------------------
# Detect the compiler
# -------------------------------------------------------------------------------------------------
set(CC_GCC 0)
set(CC_CLANG 0)
set(CC_MSVC 0)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # message("compiler is GCC")
    set(CC_GCC 1)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # message("compiler is clang")
    set(CC_CLANG 1)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # message("compiler is MSVC")
    set(CC_MSVC 1)
endif()



set(COMPILE_DEFINITIONS ${COMPILE_DEFINITIONS}

    DEBUG=$<IF:$<CONFIG:Debug>,1,0>

    # OS.
    OS_LINUX=${OS_LINUX}
    OS_MACOS=${OS_MACOS}
    OS_WINDOWS=${OS_WINDOWS}

    # Compiler.
    CC_MSVC=${CC_MSVC}
    CC_GCC=${CC_GCC}
    CC_CLANG=${CC_CLANG}


)


# Add source modules
add_subdirectory(common)


# Create the shared library combining all object libraries
add_library(datoviz SHARED
    "empty.c"
)

# Link object modules
target_link_libraries(datoviz
    PRIVATE
        datoviz_common
)

# Public headers
target_include_directories(datoviz
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_definitions(datoviz PUBLIC ${COMPILE_DEFINITIONS})

# Output name
set_target_properties(datoviz PROPERTIES OUTPUT_NAME "datoviz")

# Install rule
install(TARGETS datoviz
    LIBRARY DESTINATION lib
)
