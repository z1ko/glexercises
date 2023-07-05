
#include <iostream>
#include <cmath>

#define GLIB_INIT_IMPL
#include <initialization.hpp>

#define GLIB_INPUT_IMPL
#include <input.hpp>

#define GLIB_GRAPHICS_IMPL
#include <graphics.hpp>

#define GLIB_TRANSFORM_IMPL
#include <transform.hpp>

#include <mesh.hpp>

const char* shader_vertex = R"(

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 position;
out vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
  gl_Position = proj * view * model * vec4(aPos, 1.0);
  position = vec3(model * vec4(aPos, 1.0));
  normal = mat3(transpose(inverse(model))) * aNormal;
}

)";

const char* shader_fragment = R"(

#version 330 core
out vec4 FragColor;

in vec3 position;
in vec3 normal;

uniform vec3 cameraPos;

uniform sampler2D sampler1;

uniform struct {
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
  float shininess;
} material;

uniform struct {

  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

} light;

void main() {

  vec3 ambient = material.ambient * light.ambient;

  // Calculate diffuse light contribution
  vec3 N = normalize(normal);
  vec3 L = normalize(light.position - position);

  float kD = max(dot(N, L), 0);
  vec3 diffuse = (kD * material.diffuse) * light.diffuse;

  // Calculate specular light contribution
  vec3 V = normalize(cameraPos - position);
  vec3 R = reflect(-L, N);

  float kS = max(dot(V, R), 0);
  kS = pow(kS, material.shininess);
  vec3 specular = (kS * material.specular) * light.specular;

  FragColor = vec4(ambient + diffuse + specular, 1.0f);
}

)";

const char* shader_fragment_light = R"(

#version 330 core
out vec4 FragColor;

uniform struct {
  vec3 ambient;
} light;

void main() {
  FragColor = vec4(light.ambient, 1.0f);
}

)";

int main(int argc, char** argv) {

  GLFWwindow* window = glib::initialize(640, 480, "Window!");
  if (window == NULL) { return -1; }
  glViewport(0, 0, 640, 480);

  std::vector<float> vertices = glib::mesh_cube_with_normals();
  glib::buffer_t cube = glib::buffer_create(&vertices, NULL, [](){
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // UV
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3* sizeof(float)));
    glEnableVertexAttribArray(1);
  });

  glib::program_t program = glib::program_create(shader_vertex, shader_fragment);
  glib::program_t light_program = glib::program_create(shader_vertex, shader_fragment_light);

  // Set slot of samplers in program
  glib::program_uniform_1i(program, "sampler1", 0);

  glib::texture_t texture1 = glib::texture_load(
      "/home/z1ko/univr/graphics/data/textures/container.jpg", 
      GL_RGB, GL_REPEAT);

  const float FOV = 45.0f;
  const float ASPECT_RATIO = (float)640/480;

  glib::camera_t camera = glib::camera_base(glm::vec3(0.0f, 0.0f, 6.0f));
  glm::vec3 light_pos = glm::vec3(0.0f, 0.5f, 0.0f);

  float deltaTime = 0.0f;
  float lastFrame = 0.0f;

  glEnable(GL_DEPTH_TEST);
  while(!glfwWindowShouldClose(window)) {

    float currentTime = glfwGetTime();
    deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;

    glib::input_handle_standard(window);
    glib::camera_input(camera, window, deltaTime);

    // Update light position
    light_pos.x = sinf(glfwGetTime()) * 3.0f;
    light_pos.z = cosf(glfwGetTime()) * 3.0f;

    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set camera position for light calculation
    glib::program_uniform_3f(program, "cameraPos", 
        camera.position.x, camera.position.y, camera.position.z);

    // View matrix
    glib::program_uniform_mf(program, "view", glm::value_ptr(camera.view));
    glib::program_uniform_mf(light_program, "view", glm::value_ptr(camera.view));

    // Projection matrix
    glm::mat4 proj = glm::mat4(1.0f);
    proj = glm::perspective(glm::radians(FOV), ASPECT_RATIO, 0.1f, 100.0f);
    glib::program_uniform_mf(program, "proj", glm::value_ptr(proj));
    glib::program_uniform_mf(light_program, "proj", glm::value_ptr(proj));

    // Set material of normal cube
    glib::program_uniform_3f(program, "material.ambient",   1.0f, 0.5f, 0.31f);
    glib::program_uniform_3f(program, "material.diffuse",   1.0f, 0.5f, 0.31f);
    glib::program_uniform_3f(program, "material.specular",  0.5f, 0.5f, 0.5f );
    glib::program_uniform_1f(program, "material.shininess", 32.0f);

    // Set light properties
    glm::vec3 light_diffuse_color = glm::vec3(0.5f);
    glib::program_uniform_3f(program, "light.position", light_pos.x, light_pos.y, light_pos.z);
    glib::program_uniform_3f(program, "light.ambient",  0.2f, 0.2f, 0.2f);
    glib::program_uniform_3f(program, "light.diffuse",  light_diffuse_color.x, light_diffuse_color.y, light_diffuse_color.z);
    glib::program_uniform_3f(program, "light.specular", 1.0f, 1.0f, 1.0f);

    // Normal cube
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    glib::program_uniform_mf(program, "model", glm::value_ptr(model));
    glib::texture_bind(texture1, 0);
    glib::render(cube, program);

    // Light cube
    model = glm::mat4(1.0f);
    model = glm::translate(model, light_pos);
    model = glm::scale(model, glm::vec3(0.2f));
    glib::program_uniform_mf(light_program, "model", glm::value_ptr(model));
    glib::program_uniform_3f(light_program, "light.ambient", light_diffuse_color.x, light_diffuse_color.y, light_diffuse_color.z);
    glib::render(cube, light_program);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

