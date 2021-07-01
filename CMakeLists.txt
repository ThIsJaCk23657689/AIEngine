cmake_minimum_required(VERSION 3.19)

if (DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

if (DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT DEFINED VCPKG_TARGET_TRIPLET)
	set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}")
endif()

# Varibales setting
set(MY_PROJECT "AIEngine")
set(MY_LIBRARY "Nexus")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project (${MY_PROJECT} LANGUAGES CXX C)

add_subdirectory(External/Nexus)

# Excecutable file setting
add_executable(${MY_PROJECT} Source/Main.cpp "Source/Ball.h" "Source/Obstacle.h")
target_link_libraries(${MY_PROJECT} PUBLIC ${MY_LIBRARY})

# Copy these shader files
add_custom_command(TARGET ${MY_PROJECT} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/Shaders/ ${CMAKE_BINARY_DIR}/Shaders/)
add_custom_command(TARGET ${MY_PROJECT} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_SOURCE_DIR}/Resource/ ${CMAKE_BINARY_DIR}/Resource/)