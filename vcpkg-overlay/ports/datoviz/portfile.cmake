vcpkg_check_linkage(ONLY_DYNAMIC_LIBRARY)

if(DEFINED ENV{DATOVIZ_VCPKG_SOURCE_PATH})
    file(TO_CMAKE_PATH "$ENV{DATOVIZ_VCPKG_SOURCE_PATH}" SOURCE_PATH)
    if(NOT EXISTS "${SOURCE_PATH}/CMakeLists.txt")
        message(FATAL_ERROR "DATOVIZ_VCPKG_SOURCE_PATH does not contain CMakeLists.txt: ${SOURCE_PATH}")
    endif()
else()
    if(DEFINED ENV{DATOVIZ_VCPKG_SOURCE_URL})
        set(DATOVIZ_SOURCE_URL "$ENV{DATOVIZ_VCPKG_SOURCE_URL}")
    else()
        set(DATOVIZ_SOURCE_URL "https://github.com/datoviz/datoviz/releases/download/v${VERSION}/datoviz-${VERSION}-source.tar.gz")
    endif()

    if(DEFINED ENV{DATOVIZ_VCPKG_SOURCE_SHA512})
        set(DATOVIZ_SOURCE_SHA512 "$ENV{DATOVIZ_VCPKG_SOURCE_SHA512}")
    else()
        set(DATOVIZ_SOURCE_SHA512 "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000")
    endif()

    vcpkg_download_distfile(ARCHIVE
        URLS "${DATOVIZ_SOURCE_URL}"
        FILENAME "datoviz-${VERSION}-source.tar.gz"
        SHA512 "${DATOVIZ_SOURCE_SHA512}"
    )

    vcpkg_extract_source_archive(SOURCE_PATH
        ARCHIVE "${ARCHIVE}"
    )
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DDVZ_BUILD_TESTING=OFF
        -DDVZ_BUILD_EXAMPLES=OFF
        -DDVZ_INSTALL=ON
        -DDVZ_VENDORED_DEPS=OFF
        -DDVZ_CGLM_SOURCE=SYSTEM
        -DDVZ_MIMALLOC_SOURCE=SYSTEM
        -DDVZ_KVAZAAR_SOURCE=OFF
        -DDVZ_ENABLE_CUDA=OFF
        -DDVZ_ENABLE_QT_BRIDGE=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME datoviz CONFIG_PATH lib/cmake/datoviz)
vcpkg_fixup_pkgconfig()
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
