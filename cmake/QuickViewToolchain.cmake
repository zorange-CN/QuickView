# QuickView Toolchain Wrapper
# This file ensures that the MSVC/Windows SDK environment is detected 
# before any compiler tests or vcpkg operations occur.

# 1. Detect environment and tools
include("${CMAKE_CURRENT_LIST_DIR}/AdaptiveToolchain.cmake")

# 2. Inject detected paths into INITIAL flags (aggressive propagation)
# This helps compilers find system headers without being run from vcvars shell.
if(ADAPTIVE_MSVC_LIB_PATHS)
    foreach(path ${ADAPTIVE_MSVC_LIB_PATHS})
        if(EXISTS "${path}")
            set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} /libpath:\"${path}\"")
            set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CMAKE_SHARED_LINKER_FLAGS_INIT} /libpath:\"${path}\"")
            set(CMAKE_MODULE_LINKER_FLAGS_INIT "${CMAKE_MODULE_LINKER_FLAGS_INIT} /libpath:\"${path}\"")
        endif()
    endforeach()
endif()

if(ADAPTIVE_MSVC_INCLUDE_PATHS)
    foreach(path ${ADAPTIVE_MSVC_INCLUDE_PATHS})
        if(EXISTS "${path}")
            # Use -imsvc to suppress warnings from system headers and ensure priority
            set(CMAKE_C_FLAGS_INIT "${CMAKE_C_FLAGS_INIT} -imsvc \"${path}\"")
            set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -imsvc \"${path}\"")
        endif()
    endforeach()
endif()

# 3. Chain-load vcpkg toolchain
set(VCPKG_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/../third_party/vcpkg/scripts/buildsystems/vcpkg.cmake")
if(EXISTS "${VCPKG_TOOLCHAIN_FILE}")
    include("${VCPKG_TOOLCHAIN_FILE}")
endif()
