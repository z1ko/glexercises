cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(opengl)

cmake_policy(SET CMP0072 NEW)
find_package(OpenGL REQUIRED)

add_executable(project 
  vendor/glad/glad.c
  src/main.cpp 
)

target_link_libraries(project 
  ${CMAKE_DL_LIBS}
  OpenGL::GL
  glfw
)

target_include_directories(project
  PRIVATE "vendor"
)
