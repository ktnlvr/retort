# retort

## Design Decisions

For pure functions the arrow notation is preferred: `auto f(...) -> int`, for impure functions the standard C-notation is used.

## Building 

1. Ensure that [VulkanSDK](https://vulkan.lunarg.com/) is installed, for instance by running `vkcube`.
2. `mkdir -p build/ && cd ./build`
3. `cmake ..`
4. `retort` / `retort.exe`
