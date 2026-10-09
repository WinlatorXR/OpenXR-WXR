cmake_minimum_required(VERSION 3.16)
project(OxrWXR-Simulator VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/oxrwxr)

# For multi-config generators (Visual Studio)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/oxrwxr)
endforeach()

# Vulkan SDK — find via environment variable or fall back to D:\VulkanSDK
if(NOT DEFINED ENV{VULKAN_SDK})
    # Locate the first version folder under D:\VulkanSDK
    file(GLOB _vulkan_candidates "D:/VulkanSDK/*")
    foreach(_candidate ${_vulkan_candidates})
        if(IS_DIRECTORY "${_candidate}" AND EXISTS "${_candidate}/Include/vulkan/vulkan.h")
            set(ENV{VULKAN_SDK} "${_candidate}")
            message(STATUS "Found Vulkan SDK at: ${_candidate}")
            break()
        endif()
    endforeach()
endif()
find_package(Vulkan REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

# OpenXR Simulator Runtime DLL
add_library(openxr_wxr SHARED
    src/runtime.cpp
    src/WinXrApiUDP.cpp
)

# 32-bit builds sit next to the 64-bit dll
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set_target_properties(openxr_wxr PROPERTIES OUTPUT_NAME openxr_wxr32)
endif()

# Link libraries
target_link_libraries(openxr_wxr
    d3d11
    d3d12
    dxgi
    d3dcompiler
    dwmapi
    Vulkan::Vulkan
)

# Windows-specific settings
if(WIN32)
    target_compile_definitions(openxr_wxr PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        # Games often ship an msvcp140 older than 14.40, whose _Mtx_lock crashes on the constexpr std::mutex
        _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR
    )
endif()
