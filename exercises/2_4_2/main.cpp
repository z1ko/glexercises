
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

#include <mesh.hpp>

const char *shader_vertex = R"(

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

out vec3 frag_pos;
out vec3 normal;
out vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
  gl_Position = proj * view * model * vec4(aPos, 1.0);
  frag_pos = vec3(model * vec4(aPos, 1.0));
  normal = mat3(transpose(inverse(model))) * aNormal;
  uv = aUV;
}

)";

const char *shader_fragment = R"(

#version 330 core
out vec4 FragColor;

in vec3 frag_pos;
in vec3 normal;
in vec2 uv;

uniform vec3 cameraPos;

uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform float shininess;

uniform struct {

  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;

} light;

void main() {

  vec3 ambient = texture(diffuse_map, uv).rgb * light.ambient;

  // Calculate diffuse light contribution
  vec3 N = normalize(normal);
  vec3 L = normalize(light.position - frag_pos);

  float kD = max(dot(N, L), 0.0f);
  vec3 diffuse = kD * texture(diffuse_map, uv).rgb * light.diffuse;

  // Calculate specular light contribution
  vec3 V = normalize(cameraPos - frag_pos);
  vec3 R = reflect(-L, N);

  float kS = pow(max(dot(V, R), 0.0f), shininess);
  vec3 specular = kS * (texture(specular_map, uv).rgb) * light.specular;

  FragColor = vec4(specular + diffuse + ambient, 1.0f);
}

)";

int main(int argc, char **argv) {

  GLFWwindow *window = glib::initialize(640, 480, "Window!");
  if (window == NULL) {
    return -1;
  }
  glViewport(0, 0, 640, 480);

  std::vector<float> vertices = glib::mesh_cube_with_normals_and_uvs();
  glib::buffer_t cube = glib::buffer_create(&vertices, NULL, []() {
    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(0);
    // Color
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // UV
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                          (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
  });

  glib::program_t program =
      glib::program_create(shader_vertex, shader_fragment);

  glib::texture_t texture_diffuse = glib::texture_load(
      "../../../data/textures/container2.png", GL_RGBA, GL_REPEAT);

  glib::texture_t texture_specular =
      glib::texture_load("../../../data/textures/"
                         "lighting_maps_specular_color.png",
                         GL_RGBA, GL_REPEAT);

  const float FOV = 45.0f;
  const float ASPECT_RATIO = (float)640 / 480;

  glib::camera_t camera = glib::camera_base(glm::vec3(0.0f, 0.0f, 6.0f));
  glm::vec3 light_pos = glm::vec3(0.0f, 0.5f, 0.0f);

  float deltaTime = 0.0f;
  float lastFrame = 0.0f;

  glEnable(GL_DEPTH_TEST);
  while (!glfwWindowShouldClose(window)) {

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
    glib::program_uniform_3f(program, "cameraPos", camera.position.x,
                             camera.position.y, camera.position.z);

    // View matrix
    glm::mat4 view = glib::camera_view(camera);
    glib::program_uniform_mf(program, "view", glm::value_ptr(view));

    // Projection matrix
    glm::mat4 proj = glm::mat4(1.0f);
    proj = glm::perspective(glm::radians(FOV), ASPECT_RATIO, 0.1f, 100.0f);
    glib::program_uniform_mf(program, "proj", glm::value_ptr(proj));

    // Set material of normal cube
    glib::program_uniform_1i(program, "diffuse_map", 0);  // sampler
    glib::program_uniform_1i(program, "specular_map", 1); // sampler
    glib::program_uniform_1f(program, "shininess", 64.0f);

    // Set light properties
    glib::program_uniform_3f(program, "light.position", light_pos.x,
                             light_pos.y, light_pos.z);
    glib::program_uniform_3f(program, "light.ambient", 0.2f, 0.2f, 0.2f);
    glib::program_uniform_3f(program, "light.diffuse", 0.5f, 0.5f, 0.5f);
    glib::program_uniform_3f(program, "light.specular", 1.0f, 1.0f, 1.0f);

    // Normal cube
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
    glib::program_uniform_mf(program, "model", glm::value_ptr(model));

    glib::texture_bind(texture_diffuse, 0);  // diffuse
    glib::texture_bind(texture_specular, 1); // specular
    glib::render(cube, program);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}
