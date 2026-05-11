set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_BUILD_TYPE release)
set(VCPKG_KEEP_ENV_VARS PATH)

# [ASAN FIX] Globally inject LLVM paths into PATH so Meson can find ASan DLLs for all ports
if(DEFINED ENV{LLVM_INSTALL_DIR} AND DEFINED ENV{LLVM_VERSION})
    set(ENV{PATH} "$ENV{LLVM_INSTALL_DIR}/lib/clang/$ENV{LLVM_VERSION}/lib/windows;$ENV{LLVM_INSTALL_DIR}/bin;$ENV{PATH}")
    set(ASAN_LIB_PATH "$ENV{LLVM_INSTALL_DIR}/lib/clang/$ENV{LLVM_VERSION}/lib/windows")
else()
    # Fallback to default if not set
    set(ENV{PATH} "C:/PROGRA~1/LLVM/lib/clang/22/lib/windows;C:/PROGRA~1/LLVM/bin;$ENV{PATH}")
    set(ASAN_LIB_PATH "C:/PROGRA~1/LLVM/lib/clang/22/lib/windows")
endif()

# Force LLVM toolchain paths (using short paths to avoid space-related parsing errors)
# Dynamically find LLVM tools in PATH to get absolute paths required by vcpkg
find_program(LLVM_C_COMPILER clang-cl)
find_program(LLVM_CXX_COMPILER clang-cl)
find_program(LLVM_LINKER lld-link)
find_program(LLVM_AR llvm-lib)
find_program(LLVM_RC llvm-rc)

# Toolchain paths - expects LLVM in PATH
set(VCPKG_C_COMPILER "${LLVM_C_COMPILER}")
set(VCPKG_CXX_COMPILER "${LLVM_CXX_COMPILER}")
set(VCPKG_LINKER "${LLVM_LINKER}")
set(VCPKG_AR "${LLVM_AR}")
# Common flags (Clang-cl style)
set(ASAN_FLAGS "-fsanitize=address /Oy- -D_DISABLE_STRING_ANNOTATION -D_DISABLE_VECTOR_ANNOTATION")
set(COMMON_FLAGS "/DWIN32 /D_WINDOWS /W3 /utf-8 /Gw ${ASAN_FLAGS} /MT")
set(VCPKG_C_FLAGS "${COMMON_FLAGS}")
set(VCPKG_CXX_FLAGS "${COMMON_FLAGS}")

# Force /MT for Debug as well (Clang ASan doesn't support /MTd)
# We also remove /RTC1 as it's incompatible with ASan
set(VCPKG_C_FLAGS_DEBUG "/Z7 /Ob0 /Od")
set(VCPKG_CXX_FLAGS_DEBUG "/Z7 /Ob0 /Od")

# [SIMD FIX] -march=native is disabled here because libjxl/highway 
# fails to dispatch correctly when forced at the global level in CI-like environments.
# Individual ports should handle their SIMD targeting.

# Release optimization for ASan: Disable LTO to keep stacks clear
set(VCPKG_C_FLAGS_RELEASE "/O1 /Oi /Ob1 /DNDEBUG")
set(VCPKG_CXX_FLAGS_RELEASE "/O1 /Oi /Ob1 /DNDEBUG")

# Pure Linker flags for lld-link
# For ASan with lld-link, we must manually link the ASan runtime and thunks
set(ASAN_LIBS "clang_rt.asan_dynamic-x86_64.lib clang_rt.asan_dynamic_runtime_thunk-x86_64.lib")
set(VCPKG_LINKER_FLAGS "/LIBPATH:\"${ASAN_LIB_PATH}\" ${ASAN_LIBS}")
set(VCPKG_LINKER_FLAGS_RELEASE "/OPT:REF /OPT:ICF")

# Force internal CMake calls within vcpkg to use Clang-cl toolchain and LOCK identification
set(VCPKG_CMAKE_CONFIGURE_OPTIONS 
    "-DCMAKE_C_COMPILER=${LLVM_C_COMPILER}"
    "-DCMAKE_CXX_COMPILER=${LLVM_CXX_COMPILER}"
    "-DCMAKE_LINKER=${LLVM_LINKER}"
    "-DCMAKE_AR=${LLVM_AR}"
    "-DCMAKE_RC_COMPILER=${LLVM_RC}"
    "-DCMAKE_C_COMPILER_FRONTEND_VARIANT=MSVC"
    "-DCMAKE_CXX_COMPILER_FRONTEND_VARIANT=MSVC"
    "-DCMAKE_C_FLAGS_DEBUG=/MT /Z7 /Ob0 /Od"
    "-DCMAKE_CXX_FLAGS_DEBUG=/MT /Z7 /Ob0 /Od"
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded"
    "-DCMAKE_EXE_LINKER_FLAGS=/LIBPATH:\"${ASAN_LIB_PATH}\" ${ASAN_LIBS}"
    "-DCMAKE_SHARED_LINKER_FLAGS=/LIBPATH:\"${ASAN_LIB_PATH}\" ${ASAN_LIBS}"
    "-DCMAKE_MODULE_LINKER_FLAGS=/LIBPATH:\"${ASAN_LIB_PATH}\" ${ASAN_LIBS}"
    "-DJPEGXL_ENABLE_ENCODE=OFF"
    "-DLIBRAW_NO_JASPER=ON"
    "-DWEBP_ENABLE_ENCODE=OFF"
)

set(ZLIB_COMPAT ON)
