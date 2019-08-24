install vcpkg
install vulkan sdk
vcpkg install glm:x64-windows
vcpkg install sdl2[vulkan]:x64-windows
vcpkg install shaderc:x64-windows
vcpkg.exe integrate project
use nuget 