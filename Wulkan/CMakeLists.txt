if(MSVC)
    add_compile_options(
		/utf-8 # required for fmt
		$<$<CONFIG:Debug>:/fsanitize=address> # asan in debug mode
	) 

endif()

add_executable(Wulkan src/main.cpp)

target_include_directories(Wulkan PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/engine
	${CMAKE_CURRENT_SOURCE_DIR}/external/include
)

# TODO: move to submodules or package manager for external libraries
# change depending on build
target_link_directories(Wulkan PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/external/lib
)

target_link_libraries(Wulkan PRIVATE
    $<$<CONFIG:Debug>:fmtd.lib>
	$<$<NOT:$<CONFIG:Debug>>:fmt.lib>
)