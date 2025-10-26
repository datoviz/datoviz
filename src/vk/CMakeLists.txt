# -------------------------------------------------------------------------------------------------
# Vulkan library
# -------------------------------------------------------------------------------------------------

file(GLOB VK_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c*")
add_library(datoviz_vk OBJECT
    ${VK_SRC}
    "${CMAKE_CURRENT_SOURCE_DIR}/../../external/vk_mem_alloc.cpp"
)

target_include_directories(datoviz_vk
    PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/common
    ${PROJECT_SOURCE_DIR}/external
)

target_compile_definitions(datoviz_vk PUBLIC ${DVZ_COMPILE_DEFINITIONS})

if(WIN32)
    target_compile_definitions(datoviz_vk PRIVATE DVZ_SHARED)
endif()

# -------------------------------------------------------------------------------------------------
# Vulkan detection
# -------------------------------------------------------------------------------------------------
#
set(VULKAN_LIB "")

if(OS_LINUX)
    find_library(VULKAN_LIB vulkan)

    if(NOT VULKAN_LIB)
        message("System Vulkan loader not found, using bundled one.")
        set(VULKAN_LIB ${CMAKE_SOURCE_DIR}/libs/vulkan/linux/libvulkan.so.1)
    endif()

elseif(OS_MACOS)
    # Try system MoltenVK (Homebrew installs it in /usr/local/lib or /opt/homebrew/lib)
    find_library(VULKAN_LIB vulkan PATHS /usr/local/lib /opt/homebrew/lib)

    if(NOT VULKAN_LIB)
        message("MoltenVK not found, using bundled one.")
        set(VULKAN_LIB ${CMAKE_SOURCE_DIR}/libs/vulkan/macos/libvulkan.dylib)
    endif()

    # MoltenVK also needs to find its ICD JSON if not system-installed
    set(ENV{VK_ICD_FILENAMES} "${CMAKE_SOURCE_DIR}/libs/vulkan/macos/MoltenVK_icd.json")

elseif(OS_WINDOWS)
    # Prefer vendored import library; this will link correctly with MSVC
    set(VULKAN_LIB ${CMAKE_SOURCE_DIR}/libs/vulkan/windows/vulkan-1.lib)

    if(NOT EXISTS ${VULKAN_LIB})
        message(WARNING "vulkan-1.lib missing. Consider adding it or installing Vulkan SDK.")

        # Optional: fallback to dynamic loading approach
    endif()
endif()

# --- Attach to your target ---
target_link_libraries(datoviz_vk PRIVATE ${VULKAN_LIB})
