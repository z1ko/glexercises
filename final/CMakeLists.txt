cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(opengl)

cmake_policy(SET CMP0072 NEW)
find_package(OpenGL REQUIRED)

add_executable(final ../vendor/glad/glad.c main.cpp)

target_link_libraries(final ${CMAKE_DL_LIBS} OpenGL::GL glfw)

# Add assimp
target_link_directories(final PUBLIC "../vendor/assimp/build/bin/")
target_link_libraries(final assimp)

target_include_directories(
  final
  PRIVATE "../lib"
  PRIVATE "../vendor")
