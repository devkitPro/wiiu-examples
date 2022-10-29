cmake_minimum_required(VERSION 3.13)

# Declare project containing a single eponymous executable
project(gx2_gyro_cube LANGUAGES C CXX ASM)
add_executable(${PROJECT_NAME})
wut_create_rpx(${PROJECT_NAME})

# Find glm, which is required by this example
find_package(glm REQUIRED)

# Declare an asset target for bundled files
dkp_add_asset_target(${PROJECT_NAME}_content content)

# Add the shader to the asset dependencies
dkp_track_assets(${PROJECT_NAME}_content FILES
	shader.gsh
)

# Generate symbol list and map information (useful for debugging)
dkp_target_generate_symbol_list(${PROJECT_NAME})

# Generate a Wii U homebrew bundle
wut_create_wuhb(${PROJECT_NAME}
	NAME       "${PROJECT_NAME}"

	# Bundle a content folder
	CONTENT    ${PROJECT_NAME}_content
)

# Options for code generation
target_compile_options(${PROJECT_NAME} PRIVATE
	# Common C/C++ options
	-Wall

	# C++ specific options
	$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>
)

# Link to glm
target_link_libraries(${PROJECT_NAME} PRIVATE
	glm
)

# Source code files
target_sources(${PROJECT_NAME} PRIVATE
   source/main.cpp
)
