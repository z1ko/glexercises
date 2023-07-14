
#include <cmath>
#include <iostream>

#define GLIB_INIT_IMPL
#include <initialization.hpp>

#define GLIB_INPUT_IMPL
#include <input.hpp>

#define GLIB_GRAPHICS_IMPL
#include <graphics.hpp>

const char *shader_vertex = R"(

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aCol;
layout (location = 2) in vec2 aUV;

out vec3 color;
out vec2 uv;

void main() {
  gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
  color = aCol;
  uv = aUV;
}

)";

const char *shader_fragment = R"(

#version 330 core
out vec4 FragColor;

in vec3 color;
in vec2 uv;

uniform sampler2D sampler1;
uniform sampler2D sampler2;

void main() {
  vec4 sample1 = texture(sampler1, uv);
  vec4 sample2 = texture(sampler2, vec2(uv.x, -uv.y));

  FragColor = mix(sample1, sample2, 0.2);
}

)";

int main(int argc, char **argv) {

  GLFWwindow *window = glib::initialize(640, 480, "Window!");
  if (window == NULL) {
    return -1;
  }
  glViewport(0, 0, 640, 480);

  std::vector<float> vertices = {
      // positions          // colors           // texture coords
      0.5f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 0.65f, 0.65f,
      0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.65f, 0.35f,
      -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.35f, 0.35f,
      -0.5f, 0.5f,  0.0f, 1.0f, 1.0f, 0.0f, 0.35f, 0.65f};

  std::vector<glib::index_t> indices = {0, 1, 3, 1, 2, 3};

  glib::buffer_t triangle =
      glib::buffer_create(&vertices, &indices, glib::texture_layout);
  glib::program_t program =
      glib::program_create(shader_vertex, shader_fragment);

  // Set slot of samplers in program
  glib::program_uniform_1i(program, "sampler1", 0);
  glib::program_uniform_1i(program, "sampler2", 1);

  glib::texture_t texture1 = glib::texture_load(
      "../../../data/textures/container.jpg", GL_RGB, GL_REPEAT);
  glib::texture_t texture2 = glib::texture_load(
      "../../../data/textures/awesomeface.png", GL_RGBA, GL_REPEAT);

  float elapsed = 0.0f, offset = 0.0f;
  while (!glfwWindowShouldClose(window)) {
    elapsed += 0.01f;

    glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glib::input_handle_standard(window);

    glib::texture_bind(texture1, 0);
    glib::texture_bind(texture2, 1);
    glib::render(triangle, program);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
