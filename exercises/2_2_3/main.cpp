
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
out vec3 color;

// World-space
uniform vec3 lightPos = vec3(2.0f, 2.0f, 5.f);
uniform vec3 lightCol = vec3(0.5f, 0.25f, 0.75f);
uniform vec3 cameraPos = vec3(0.0f, 0.0f, 6.0f);

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {

  gl_Position = proj * view * model * vec4(aPos, 1.0);
  position = vec3(model * vec4(aPos, 1.0));
  normal = mat3(transpose(inverse(model))) * aNormal;
  
  float ambientStrength = 0.2;
  vec3 ambient = ambientStrength * vec3(1.0f);

  // Calculate diffuse light contribution
  vec3 N = normalize(normal);
  vec3 L = normalize(lightPos - position);

  float kD = max(dot(N, L), 0);
  vec3 diffuse = kD * lightCol;

  // Calculate specular light contribution
  vec3 V = normalize(cameraPos - position);
  vec3 R = reflect(-L, N);

  float specularStrength = 1.0f;
  float kS = max(dot(V, R), 0);
  kS = pow(kS, 32);
  vec3 specular = specularStrength * kS * lightCol;

  color = ambient + diffuse + specular;
}

)";

const char* shader_fragment = R"(

#version 330 core
out vec4 FragColor;

in vec3 position;
in vec3 normal;
in vec3 color;


void main() {
  vec3 baseColor = vec3(0.75f, 0.25f, 0.5f);
  FragColor = vec4(baseColor * color, 1.0f);
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
    glib::program_uniform_3f(program, "lightPos",
        light_pos.x, light_pos.y, light_pos.z);

    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set camera position for light calculation
    glib::program_uniform_3f(program, "cameraPos", 
        camera.position.x, camera.position.y, camera.position.z);

    // View matrix
    glib::program_uniform_mf(program, "view", glm::value_ptr(camera.view));

    // Projection matrix
    glm::mat4 proj = glm::mat4(1.0f);
    proj = glm::perspective(glm::radians(FOV), ASPECT_RATIO, 0.1f, 100.0f);
    glib::program_uniform_mf(program, "proj", glm::value_ptr(proj));

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    glib::program_uniform_mf(program, "model", glm::value_ptr(model));

    glib::texture_bind(texture1, 0);
    glib::render(cube, program);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

