# QuickView Toolchain Wrapper
# This file ensures that the MSVC/Windows SDK environment is detected 
# before any compiler tests or vcpkg operations occur.

# 1. Detect environment and tools
include("${CMAKE_CURRENT_LIST_DIR}/AdaptiveToolchain.cmake")

# 2. (Removed) Environments are now injected directly via ENV{LIB} and ENV{INCLUDE} in AdaptiveToolchain.cmake

# 3. Chain-load vcpkg toolchain
set(VCPKG_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../third_party/vcpkg/scripts/buildsystems/vcpkg.cmake")
if(EXISTS "${VCPKG_TOOLCHAIN_FILE}")
    include("${VCPKG_TOOLCHAIN_FILE}")
endif()
