vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO AOMediaCodec/libavif
    REF "v${VERSION}"
    SHA512 103a54cbc20a1ae1c948bf5e09cb7696b966c55445f9fd951207fbf651039ebd56e0ad04a6fdf5538299af5ee96339a760fe1cb904af47f51072642a750fd699
    HEAD_REF master
    PATCHES
        dependencies.diff
        disable-source-utf8.patch
)
vcpkg_replace_string("${SOURCE_PATH}/cmake/Modules/merge_static_libs.cmake"
    "elseif(CMAKE_C_COMPILER_ID MATCHES \"^(Clang|GNU|Intel|IntelLLVM)$\")"
    "elseif(CMAKE_C_COMPILER_ID MATCHES \"^(Clang|GNU|Intel|IntelLLVM)$\" AND NOT MSVC)"
)
# When using clang-cl, CMAKE_AR is llvm-lib but libavif only searches for 'lib.exe'.
# Patch to use CMAKE_AR as the primary choice for the MSVC branch.
vcpkg_replace_string("${SOURCE_PATH}/cmake/Modules/merge_static_libs.cmake"
    "find_program(BUNDLE_TOOL lib HINTS \"\${CMAKE_C_COMPILER}/..\")"
    "find_program(BUNDLE_TOOL NAMES lib llvm-lib HINTS \"\${CMAKE_C_COMPILER}/..\" \"\${CMAKE_AR}/..\")"
)
file(REMOVE_RECURSE "${SOURCE_PATH}/third_party")

set(FEATURE_OPTIONS "")
if("aom" IN_LIST FEATURES)
    list(APPEND FEATURE_OPTIONS "-DAVIF_CODEC_AOM=SYSTEM")
endif()
if("dav1d" IN_LIST FEATURES)
    list(APPEND FEATURE_OPTIONS "-DAVIF_CODEC_DAV1D=SYSTEM")
endif()

vcpkg_find_acquire_program(PKGCONFIG)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
        "-DPKG_CONFIG_EXECUTABLE=${PKGCONFIG}"
        -DAVIF_BUILD_APPS=OFF
        -DAVIF_BUILD_TESTS=OFF
        -DAVIF_ENABLE_WERROR=OFF
        -DAVIF_CODEC_AOM_ENCODE=OFF
        -DAVIF_CODEC_RAV1E=OFF
        -DAVIF_CODEC_SVT=OFF
)
vcpkg_cmake_install()
vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/${PORT})

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include"
                    "${CURRENT_PACKAGES_DIR}/debug/share")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
