
#include <iostream>

#define GLIB_INIT_IMPL
#include <initialization.hpp>

#define GLIB_INPUT_IMPL
#include <input.hpp>

#define GLIB_GRAPHICS_IMPL
#include <graphics.hpp>

const char* shader_vertex = R"(

#version 330 core
layout (location = 0) in vec3 aPos;
void main() {
  gl_Position = vec4(aPos.xyz, 1.0);
}

)";

const char* shader_fragment = R"(

#version 330 core
out vec4 FragColor;
void main() {
  FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}

)";

int main(int argc, char** argv) {

  GLFWwindow* window = glib::initialize(640, 480, "Window!");
  if (window == NULL) { return -1; }
  glViewport(0, 0, 640, 480);

  std::vector<float> vertices1 = { 
    // first triangle
    -0.9f, -0.5f, 0.0f,
    -0.0f, -0.5f, 0.0f,
    -0.45f, 0.5f, 0.0f,
  };

  std::vector<float> vertices2 = {
    // second triangle
     0.0f, -0.5f, 0.0f,
     0.9f, -0.5f, 0.0f,
     0.45f, 0.5f, 0.0f
  };

  glib::buffer_t triangle1 = glib::buffer_create(&vertices1, NULL);
  glib::buffer_t triangle2 = glib::buffer_create(&vertices2, NULL);

  glib::program_t program = glib::program_create(shader_vertex, shader_fragment);

  glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
  while(!glfwWindowShouldClose(window)) {
    glClear(GL_COLOR_BUFFER_BIT);

    glib::input_handle_standard(window);
    glib::render(triangle1, program);
    glib::render(triangle2, program);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

