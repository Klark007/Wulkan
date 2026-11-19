if(MSVC)
    add_compile_options(
		/utf-8 # required for fmt
		$<$<CONFIG:Debug>:/fsanitize=address> # asan in debug mode
		/W4
		/sdl # security warnings are erros
		/MP  # multiprocess build
		/permissive- # conform to standard

		/DEBUG       # Generate Debug Info
		/external:W1

		$<$<NOT:$<CONFIG:Debug>>:/Oi> # Enable intrinsic functions
		$<$<NOT:$<CONFIG:Debug>>:/Gy> # Enable function level linking 
		$<$<NOT:$<CONFIG:Debug>>:/GL> # Whole program optimizations

		$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF> # Enable COMDAT Folding
		$<$<NOT:$<CONFIG:Debug>>:/OPT:REF> # References
		
	) 

	# TODO TRACY enabling
	add_compile_definitions(
		_CONSOLE
	)

endif()

add_compile_definitions(
	$<$<CONFIG:Debug>:_DEBUG>
	$<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)

set(IMGUI_DIR ${PROJECT_SOURCE_DIR}/Wulkan/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
	${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

file(GLOB_RECURSE WULKAN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE WULKAN_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

add_executable(Wulkan 
	${WULKAN_SOURCES} ${WULKAN_HEADERS}
	
	${IMGUI_SOURCES}
	${PROJECT_SOURCE_DIR}/Wulkan/external/miniz/miniz.c
	${PROJECT_SOURCE_DIR}/Wulkan/external/tracy/public/TracyClient.cpp
	${PROJECT_SOURCE_DIR}/Wulkan/external/vk-bootstrap/VkBootstrap.cpp
	${PROJECT_SOURCE_DIR}/Wulkan/external/Volk/volk.c
)

target_include_directories(Wulkan PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/engine

	"C:/VulkanSDK/1.4.321.1/Include"
	${CMAKE_CURRENT_SOURCE_DIR}/external/include
	${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
	${CMAKE_CURRENT_SOURCE_DIR}/external/miniz
	${CMAKE_CURRENT_SOURCE_DIR}/external/tracy/public
	${CMAKE_CURRENT_SOURCE_DIR}/external/vk-bootstrap
	${CMAKE_CURRENT_SOURCE_DIR}/external/Volk
	${CMAKE_CURRENT_SOURCE_DIR}/external # Necessary for imgui_impl_vulkan include volk; looks for Volk/volk.h
)

# TODO: move to submodules or package manager for external libraries
# change depending on build
target_link_directories(Wulkan PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/external/lib
	"C:/VulkanSDK/1.4.321.1/Lib"
)

target_link_libraries(Wulkan PRIVATE
    $<$<CONFIG:Debug>:fmtd.lib>
	$<$<NOT:$<CONFIG:Debug>>:fmt.lib>

	$<$<CONFIG:Debug>:glfw3d.lib>
	$<$<NOT:$<CONFIG:Debug>>:glfw3.lib>

	vulkan-1.lib
)

target_precompile_headers(Wulkan PRIVATE 
    "$<$<COMPILE_LANGUAGE:CXX>:common.h>"
)