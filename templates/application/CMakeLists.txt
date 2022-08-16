cmake_minimum_required(VERSION 3.13)

# Declare project containing a single eponymous executable
project(wiiu_application LANGUAGES C CXX ASM)
add_executable(${PROJECT_NAME} "")
wut_create_rpx(${PROJECT_NAME})

# Declare an asset target for bundled files (optional)
#dkp_add_asset_target(${PROJECT_NAME}_content content)

# Generate symbol list and map information (useful for debugging)
dkp_target_generate_symbol_list(${PROJECT_NAME})

# Generate a Wii U homebrew bundle
wut_create_wuhb(${PROJECT_NAME}
	NAME       "${PROJECT_NAME}"

	# Bundle a content folder (optional)
	#CONTENT    ${PROJECT_NAME}_content

	# Other options available:
	#SHORTNAME  "${PROJECT_NAME}"
	#AUTHOR     "Built with devkitPPC & wut"
	#ICON       myIcon.png
	#TVSPLASH   myTvSplash.png
	#DRCSPLASH  myDrcSplash.png
)

# Options for code generation
target_compile_options(${PROJECT_NAME} PRIVATE
	# Common C/C++ options
	-Wall

	# C++ specific options
	$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>
)

# Options for linking to libraries (example)
#target_link_libraries(${PROJECT_NAME} PRIVATE
#	somelib
#)

# Source code files
target_sources(${PROJECT_NAME} PRIVATE
	source/main.c
)
