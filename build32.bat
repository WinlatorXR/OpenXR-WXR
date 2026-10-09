rmdir /S /Q build32
set VULKAN_LIB=%VULKAN_SDK%\Lib32\vulkan-1.lib
if exist "%VULKAN_LIB%" goto runtime
cmake -S vulkan32 -B build32\vulkan32 -G "Visual Studio 17 2022" -A Win32
cmake --build build32\vulkan32 --config Release
set VULKAN_LIB=%CD%\build32\vulkan32\Release\vulkan-1.lib
:runtime
cmake -S . -B build32 -G "Visual Studio 17 2022" -A Win32 "-DVulkan_LIBRARY=%VULKAN_LIB%"
cmake --build build32 --config Release
pause
