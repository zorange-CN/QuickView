set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

# Toolchain
set(VCPKG_C_COMPILER clang-cl)
set(VCPKG_CXX_COMPILER clang-cl)
set(VCPKG_LINKER lld-link)
set(VCPKG_AR llvm-lib)

# Triple for cross-compilation
set(VCPKG_C_FLAGS "--target=arm64-pc-windows-msvc /DWIN32 /D_WINDOWS /W3 /utf-8 /Gw")
set(VCPKG_CXX_FLAGS "--target=arm64-pc-windows-msvc /DWIN32 /D_WINDOWS /W3 /utf-8 /Gw")

# Release flags
set(VCPKG_C_FLAGS_RELEASE "/O2 /Oi /Ob2 -flto /DNDEBUG")
set(VCPKG_CXX_FLAGS_RELEASE "/O2 /Oi /Ob2 -flto /DNDEBUG")

# Optimization for size and performance
if(NOT PORT STREQUAL "libraw")
    string(APPEND VCPKG_C_FLAGS_RELEASE " /GR- /EHs-c-")
    string(APPEND VCPKG_CXX_FLAGS_RELEASE " /GR- /EHs-c-")
endif()

# Let vcpkg handle the machine type and other linker flags automatically
set(VCPKG_LINKER_FLAGS_RELEASE "/OPT:REF /OPT:ICF")

set(VCPKG_CMAKE_CONFIGURE_OPTIONS 
    "-DCMAKE_SYSTEM_NAME=Windows"
    "-DCMAKE_SYSTEM_PROCESSOR=ARM64"
    "-DJPEGXL_ENABLE_ENC=OFF"
    "-DJPEGXL_ENABLE_BENCHMARK=OFF"
    "-DJPEGXL_ENABLE_EXAMPLES=OFF"
    "-DLIBRAW_NO_JASPER=ON"
    "-DWEBP_BUILD_ENCODER=OFF"
    "-DWEBP_BUILD_CWEBP=OFF"
    "-DWEBP_BUILD_GIF2WEBP=OFF"
    "-DWEBP_BUILD_IMG2WEBP=OFF"
    "-DWEBP_BUILD_EXTRAS=OFF"
    "-DBROTLI_DISABLE_ENCODER=ON"
)

set(ZLIB_COMPAT ON)
