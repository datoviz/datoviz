vcpkg_check_linkage(ONLY_DYNAMIC_LIBRARY)

vcpkg_download_distfile(ARCHIVE
    URLS "https://github.com/datoviz/datoviz/releases/download/v${VERSION}/datoviz-${VERSION}-source.tar.gz"
    FILENAME "datoviz-${VERSION}-source.tar.gz"
    SHA512 00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
)

vcpkg_extract_source_archive(SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
)

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

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
