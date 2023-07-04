
#include <iostream>
#include <cmath>

#define GLIB_INIT_IMPL
#include <initialization.hpp>

#define GLIB_INPUT_IMPL
#include <input.hpp>

#define GLIB_GRAPHICS_IMPL
#include <graphics.hpp>

const char* shader_vertex = R"(

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aCol;

uniform float offset;
out vec3 color;

void main() {
  gl_Position = vec4(aPos.x + offset, aPos.y, aPos.z, 1.0);
  color = aCol;
}

)";

const char* shader_fragment = R"(

#version 330 core
out vec4 FragColor;

in vec3 color;

void main() {
  FragColor = vec4(color.xyz, 1.0f);
}

)";

int main(int argc, char** argv) {

  GLFWwindow* window = glib::initialize(640, 480, "Window!");
  if (window == NULL) { return -1; }
  glViewport(0, 0, 640, 480);

  std::vector<float> vertices = {
    // Position         Color
     0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,
    -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,
     0.0f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f
  };

  std::vector<glib::index_t> indices = {
    0, 1, 2,
  };

  glib::buffer_t triangle = glib::buffer_create(&vertices, &indices, glib::color_layout);
  glib::program_t program = glib::program_create(shader_vertex, shader_fragment);

  float elapsed = 0.0f, offset = 0.0f;
  while(!glfwWindowShouldClose(window)) {
    elapsed += 0.01f;

    glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Update uniform
    offset = sinf(elapsed) / 2.0f;
    glib::program_uniform_1f(program, "offset", offset);

    glib::input_handle_standard(window);
    glib::render(triangle, program);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

