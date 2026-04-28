cmake_minimum_required(VERSION 3.16)
project(skeletal_animation_min C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(ASSIMP REQUIRED)

add_library(glad STATIC src_support/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/includes)

add_library(stb_image STATIC src_support/stb_image.cpp)
target_include_directories(stb_image PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/includes)

add_executable(8.guest_2020_skeletal_animation
  src/8.guest/2020/skeletal_animation/skeletal_animation.cpp
)

target_include_directories(8.guest_2020_skeletal_animation PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/includes
  ${CMAKE_CURRENT_SOURCE_DIR}/configuration
  ${ASSIMP_INCLUDE_DIR}
)

# GLFW is expected to be installed on the system (e.g. Homebrew).
find_library(GLFW_LIB glfw3)
if (NOT GLFW_LIB)
  message(FATAL_ERROR "glfw3 library not found. Install glfw (e.g. via Homebrew) and reconfigure.")
endif()

target_link_libraries(8.guest_2020_skeletal_animation PRIVATE
  glad
  stb_image
  ${GLFW_LIB}
  ${OPENGL_gl_LIBRARY}
  ${ASSIMP_LIBRARY}
)

