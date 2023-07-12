cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(opengl)

cmake_policy(SET CMP0072 NEW)
find_package(OpenGL REQUIRED)

# Add assimp support
add_subdirectory(../../vendor/assimp/ ./assimp)
add_executable(exercise ../../vendor/glad/glad.c main.cpp)

target_link_libraries(exercise ${CMAKE_DL_LIBS} assimp::assimp OpenGL::GL glfw)

target_include_directories(
  exercise
  PRIVATE "../../lib"
  PRIVATE "../../vendor")
