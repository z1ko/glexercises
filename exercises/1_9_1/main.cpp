
#include <cmath>
#include <iostream>

#define GLIB_INIT_IMPL
#include <initialization.hpp>

#define GLIB_INPUT_IMPL
#include <input.hpp>

#define GLIB_GRAPHICS_IMPL
#include <graphics.hpp>

#define GLIB_TRANSFORM_IMPL
#include <transform.hpp>

const char *shader_vertex = R"(

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;

out vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
  gl_Position = proj * view * model * vec4(aPos.x, aPos.y, aPos.z, 1.0);
  uv = aUV;
}

)";

const char *shader_fragment = R"(

#version 330 core
out vec4 FragColor;

in vec2 uv;

uniform sampler2D sampler1;

void main() {
  vec4 sample1 = texture(sampler1, uv);
  FragColor = sample1;
}

)";

int main(int argc, char **argv) {

  GLFWwindow *window = glib::initialize(640, 480, "Window!");
  if (window == NULL) {
    return -1;
  }
  glViewport(0, 0, 640, 480);

  std::vector<float> vertices = {
      -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 0.0f,
      0.5f,  0.5f,  -0.5f, 1.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 1.0f, 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
      -0.5f, 0.5f,  0.5f,  0.0f, 1.0f, -0.5f, -0.5f, 0.5f,  0.0f, 0.0f,

      -0.5f, 0.5f,  0.5f,  1.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 1.0f, 1.0f,
      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  0.5f,  1.0f, 0.0f,

      0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      0.5f,  -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 0.0f, 1.0f,
      0.5f,  -0.5f, 0.5f,  0.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

      -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.5f,  -0.5f, -0.5f, 1.0f, 1.0f,
      0.5f,  -0.5f, 0.5f,  1.0f, 0.0f, 0.5f,  -0.5f, 0.5f,  1.0f, 0.0f,
      -0.5f, -0.5f, 0.5f,  0.0f, 0.0f, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

      -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.5f,  0.5f,  -0.5f, 1.0f, 1.0f,
      0.5f,  0.5f,  0.5f,  1.0f, 0.0f, 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
      -0.5f, 0.5f,  0.5f,  0.0f, 0.0f, -0.5f, 0.5f,  -0.5f, 0.0f, 1.0f};

  glib::buffer_t cube = glib::buffer_create(&vertices, NULL, []() {
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    // UV
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
  });
  glib::program_t program =
      glib::program_create(shader_vertex, shader_fragment);

  // Set slot of samplers in program
  glib::program_uniform_1i(program, "sampler1", 0);

  glib::texture_t texture1 = glib::texture_load(
      "../../../data/textures/container.jpg", GL_RGB, GL_REPEAT);

  std::vector<glm::vec3> cubePositions = {
      glm::vec3(0.0f, 0.0f, 0.0f),    glm::vec3(2.0f, 5.0f, -15.0f),
      glm::vec3(-1.5f, -2.2f, -2.5f), glm::vec3(-3.8f, -2.0f, -12.3f),
      glm::vec3(2.4f, -0.4f, -3.5f),  glm::vec3(-1.7f, 3.0f, -7.5f),
      glm::vec3(1.3f, -2.0f, -2.5f),  glm::vec3(1.5f, 2.0f, -2.5f),
      glm::vec3(1.5f, 0.2f, -1.5f),   glm::vec3(-1.3f, 1.0f, -1.5f)};

  const float FOV = 45.0f;
  const float ASPECT_RATIO = (float)640 / 480;

  glib::camera_t camera = glib::camera_base(glm::vec3(0.0f, 0.0f, 6.0f));

  float deltaTime = 0.0f;
  float lastFrame = 0.0f;

  glEnable(GL_DEPTH_TEST);
  while (!glfwWindowShouldClose(window)) {

    float currentTime = glfwGetTime();
    deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;

    glib::input_handle_standard(window);
    glib::camera_input(camera, window, deltaTime);
    // glib::camera_look_at(camera, cubePositions[0]);

    glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // View matrix
    glm::mat4 view = glib::camera_view(camera);
    glib::program_uniform_mf(program, "view", glm::value_ptr(view));

    // Projection matrix
    glm::mat4 proj = glm::mat4(1.0f);
    proj = glm::perspective(glm::radians(FOV), ASPECT_RATIO, 0.1f, 100.0f);
    glib::program_uniform_mf(program, "proj", glm::value_ptr(proj));

    for (int i = 0; i < cubePositions.size(); ++i) {

      float angle = 20.0f * i;
      if (i % 3 == 0) {
        angle = (float)glfwGetTime() * 20.0f;
      }

      // Model
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, cubePositions[i]);
      model =
          glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
      glib::program_uniform_mf(program, "model", glm::value_ptr(model));

      glib::texture_bind(texture1, 0);
      glib::render(cube, program);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
