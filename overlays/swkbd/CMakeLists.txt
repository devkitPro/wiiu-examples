cmake_minimum_required(VERSION 3.13)

# Declare project containing a single eponymous executable
project(swkbd LANGUAGES C CXX ASM)
add_executable(${PROJECT_NAME})
wut_create_rpx(${PROJECT_NAME})

# Generate symbol list and map information (useful for debugging)
dkp_target_generate_symbol_list(${PROJECT_NAME})

# Generate a Wii U homebrew bundle
wut_create_wuhb(${PROJECT_NAME}
	NAME       "${PROJECT_NAME}"
)

# Options for code generation
target_compile_options(${PROJECT_NAME} PRIVATE
	# Common C/C++ options
	-Wall

	# C++ specific options
	$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>
)

# Source code files
target_sources(${PROJECT_NAME} PRIVATE
   main.cpp
)
