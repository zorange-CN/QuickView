vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO google/highway
    REF "${VERSION}"
    SHA512 8b9f4fdc4fa60b6817417959853f5b55bf86aec9d35fc6664dda15179cc55e0a9940f3a46011a84b95263ba342dc47ca1cb93b04481ff4b63d724cce1815d7c6
    HEAD_REF master
    PATCHES
        2695.patch
)

# [FIX] Clang-cl 22.1.5 rejects "evex512" and "avx10.2-512" in target attribute, causing the entire attribute to be ignored.
# This leads to "always_inline function requires target feature 'avx512f'" errors.
vcpkg_replace_string("${SOURCE_PATH}/hwy/ops/set_macros-inl.h"
    "HWY_COMPILER_CLANG >= 1800"
    "HWY_COMPILER_CLANG >= 1800 && !HWY_COMPILER_CLANGCL"
)
vcpkg_replace_string("${SOURCE_PATH}/hwy/ops/set_macros-inl.h"
    "HWY_COMPILER_CLANG >= 2000"
    "HWY_COMPILER_CLANG >= 2000 && !HWY_COMPILER_CLANGCL"
)

# Inject HWY_BROKEN_CLANGCL into detect_targets.h
vcpkg_replace_string("${SOURCE_PATH}/hwy/detect_targets.h"
    "#ifndef HWY_BROKEN_AVX3_SPR"
    "#ifndef HWY_BROKEN_CLANGCL\n#if HWY_COMPILER_CLANGCL\n#define HWY_BROKEN_CLANGCL (HWY_AVX10_2 | HWY_AVX3_SPR)\n#else\n#define HWY_BROKEN_CLANGCL 0\n#endif\n#endif\n\n#ifndef HWY_BROKEN_AVX3_SPR"
)
vcpkg_replace_string("${SOURCE_PATH}/hwy/detect_targets.h"
    "HWY_BROKEN_LOONGARCH | HWY_BROKEN_Z14)"
    "HWY_BROKEN_LOONGARCH | HWY_BROKEN_Z14 | HWY_BROKEN_CLANGCL)"
)


vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        contrib  HWY_ENABLE_CONTRIB
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
        -DHWY_ENABLE_INSTALL=ON
        -DHWY_ENABLE_EXAMPLES=OFF
        -DHWY_ENABLE_TESTS=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME hwy CONFIG_PATH lib/cmake/hwy)

if(VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/include/hwy/highway_export.h" "defined(HWY_SHARED_DEFINE)" "1")
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_fixup_pkgconfig()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
