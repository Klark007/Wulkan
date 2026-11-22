if(MSVC)
    add_compile_options(
		/utf-8 # required for fmt
		$<$<CONFIG:Debug>:/fsanitize=address> # asan in debug mode
		/W4
		/sdl # security warnings are erros
		/MP  # multiprocess build
		/permissive- # conform to standard

		/external:W1

		$<$<NOT:$<CONFIG:Debug>>:/Oi> # Enable intrinsic functions
		$<$<NOT:$<CONFIG:Debug>>:/Gy> # Enable function level linking 
		$<$<NOT:$<CONFIG:Debug>>:/GL> # Whole program optimizations
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

set(EXT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external) # relative to current folder
set(IMGUI_DIR ${EXT_DIR}/imgui)
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
	${EXT_DIR}/miniz/miniz.c
	${EXT_DIR}/vk-bootstrap/VkBootstrap.cpp
	${EXT_DIR}/Volk/volk.c
)

if(WULKAN_TRACY)
	set(TRACY_ENABLE ${WULKAN_TRACY} CACHE BOOL "" FORCE)
	add_subdirectory(${EXT_DIR}/tracy SYSTEM)
	target_link_libraries(Wulkan PRIVATE Tracy::TracyClient)
	
	#target_compile_definitions(Wulkan PRIVATE
    #    WIN32_LEAN_AND_MEAN
    #)
endif()

if (MSVC)
	target_compile_options(Wulkan PRIVATE /Zi) # Generate Debug Info

	target_link_options(Wulkan PRIVATE /DEBUG

		$<$<CONFIG:Debug>:/INCREMENTAL:NO> # required by ASan
		$<$<NOT:$<CONFIG:Debug>>:/LTCG> # Whole program optimizations
		$<$<NOT:$<CONFIG:Debug>>:/OPT:ICF> # Enable COMDAT Folding
		$<$<NOT:$<CONFIG:Debug>>:/OPT:REF> # References
	)
endif()

target_include_directories(Wulkan PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/engine
)

# actual external libraries with lesser warn level
target_include_directories(Wulkan SYSTEM PRIVATE 
	"C:/VulkanSDK/1.4.321.1/Include"
	${EXT_DIR}/include
	${EXT_DIR}/imgui
	${EXT_DIR}/miniz
	${EXT_DIR}/vk-bootstrap
	${EXT_DIR}/Volk
	${EXT_DIR} # Necessary for imgui_impl_vulkan include volk; looks for Volk/volk.h
)

# TODO: move to submodules or package manager for external libraries
# change depending on build
target_link_directories(Wulkan PRIVATE
	${EXT_DIR}/lib
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
    "$<$<COMPILE_LANGUAGE:CXX>:common.h>" # shouldn't be added to files that use C compiler
)

# copy shaders and model to output path
add_custom_command(TARGET Wulkan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/models $<TARGET_FILE_DIR:Wulkan>/models
	COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/textures $<TARGET_FILE_DIR:Wulkan>/textures
)

# compile shaders 
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

find_program(GLSLC  glslc  HINTS $ENV{VULKAN_SDK}/Bin REQUIRED)
find_program(SLANGC slangc HINTS $ENV{VULKAN_SDK}/Bin REQUIRED)

file(GLOB_RECURSE SHADERS CONFIGURE_DEPENDS
    ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag
    ${SHADER_DIR}/*.tesc ${SHADER_DIR}/*.tese
    ${SHADER_DIR}/*.comp ${SHADER_DIR}/*.slang
)

set(SPV_FILES "")
foreach(SHADER ${SHADERS})
    file(RELATIVE_PATH REL ${SHADER_DIR} ${SHADER})
	get_filename_component(REL_DIR ${REL} DIRECTORY)
    get_filename_component(NAME    ${SHADER} NAME_WLE)
    get_filename_component(EXT     ${SHADER} LAST_EXT)
	string(SUBSTRING ${EXT} 1 -1 STAGE)

	set(OUT_DIR ${SHADER_OUT_DIR}/${REL_DIR})
    # Debug -> name_dvert.spv, otherwise name_vert.spv (same naming as compile.bat)
    set(OUT ${OUT_DIR}/${NAME}_$<$<CONFIG:Debug>:d>${STAGE}.spv)

	if(STAGE STREQUAL "slang")
		set(SHADER_CMT "[Slang] Compiling: ${REL} to ${OUT}")
		set(SHADER_CMD ${SLANGC} ${SHADER} -target spirv $<IF:$<CONFIG:Debug>,-g,-O3> -o ${OUT})
	else()
		set(SHADER_CMT "[Glslc] Compiling: ${REL} to ${OUT}")
		set(SHADER_CMD ${GLSLC} ${SHADER} --target-env=vulkan1.3 $<IF:$<CONFIG:Debug>,-g,-O> -o ${OUT})
	endif()

	add_custom_command(
        OUTPUT ${OUT}											# produces out
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUT_DIR}   # create if not existant
        COMMAND ${SHADER_CMD}											# compiles shader
        DEPENDS ${SHADER}										# depending on shader file
		COMMENT ${SHADER_CMT}
        VERBATIM
    )
	list(APPEND SPV_FILES ${OUT})  
endforeach()

# depend on compiled files
add_custom_target(Shaders DEPENDS ${SPV_FILES})
add_dependencies(Wulkan Shaders)