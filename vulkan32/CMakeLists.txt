# x86 import library for vulkan-1.dll, for SDKs installed without the 32-bit libraries
cmake_minimum_required(VERSION 3.16)
project(vulkan32 LANGUAGES C)
add_library(vulkan-1 SHARED vulkan_stub.c vulkan-1.def)
