cmake_minimum_required (VERSION 3.24)
set (CMAKE_CXX_STANDARD 23)
include(FetchContent)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("retort")

add_executable (retort "src/main.cpp")

find_package(Vulkan REQUIRED)

set(VK_BOOTSTRAP_VERSION 1.3.302)
FetchContent_Declare(
	fetch_vk_bootstrap
	URL https://github.com/charles-lunarg/vk-bootstrap/archive/refs/tags/v${VK_BOOTSTRAP_VERSION}.tar.gz
	FIND_PACKAGE_ARGS ${VK_BOOTSTRAP_VERSION}
)
FetchContent_MakeAvailable(fetch_vk_bootstrap)

FetchContent_Declare(
	fetch_glfw
	GIT_REPOSITORY https://github.com/glfw/glfw
)
FetchContent_MakeAvailable(fetch_glfw)

set(SHADERC_SKIP_TESTS ON)
FetchContent_Declare(
	fetch_shaderc
	GIT_REPOSITORY https://github.com/google/shaderc
	GIT_TAG		   origin/main
)
FetchContent_MakeAvailable(fetch_shaderc)

FetchContent_Declare(imgui_external
	URL https://github.com/ocornut/imgui/archive/refs/tags/v1.90.8.tar.gz
	EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(imgui_external)

add_library(imgui
	${imgui_external_SOURCE_DIR}/imgui.cpp
	${imgui_external_SOURCE_DIR}/imgui_draw.cpp
	${imgui_external_SOURCE_DIR}/imgui_tables.cpp
	${imgui_external_SOURCE_DIR}/imgui_widgets.cpp
	# TODO: remove me when done testing
	${imgui_external_SOURCE_DIR}/imgui_demo.cpp
	${imgui_external_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
	${imgui_external_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)
target_include_directories(imgui PUBLIC ${imgui_external_SOURCE_DIR} PUBLIC ${VULKAN_INCLUDE_DIRS} PUBLIC ${GLFW_INCLUDE_DIRS})
target_link_libraries(imgui vk-bootstrap::vk-bootstrap glfw Vulkan::Vulkan)

target_link_libraries(retort vk-bootstrap::vk-bootstrap glfw shaderc_shared imgui)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET retort PROPERTY CXX_STANDARD 20)
endif()
