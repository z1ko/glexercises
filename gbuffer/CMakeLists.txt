cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(opengl)

cmake_policy(SET CMP0072 NEW)
find_package(OpenGL REQUIRED)

add_definitions("-lassimp")
find_package(ASSIMP REQUIRED)
if(ASSIMP_FOUND)
    include_directories(${ASSIMP_INCLUDE_DIR})
endif() 

add_executable(gbuffer
  ../vendor/glad/glad.c
  main.cpp 
)

target_link_libraries(gbuffer
  ${CMAKE_DL_LIBS}
  ${ASSIMP_LIBRARY_DIRS}/libassimp.so
  OpenGL::GL
  glfw
)

target_include_directories(gbuffer
  PRIVATE "../lib"
  PRIVATE "../vendor"
)
