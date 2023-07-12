
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

#define GLIB_MODEL_IMPL
#include <model.hpp>

const int WIDTH = 1600;
const int HEIGHT = 900;

const char *shader_vertex_light = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
  gl_Position = proj * view * model * vec4(aPos, 1.0);
}

)";

const char *shader_vertex = R"(

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec2 aUV;

out vec2 uv;
out vertex_output_tangent {
  
  vec3 frag_pos;
  vec3 camera_pos;
  vec3 sun_dir;
  vec3 light_pos;

} tspace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

uniform vec3 camera_pos;
uniform vec3 sun_dir;
uniform vec3 light_pos;

void main() {
  gl_Position = proj * view * model * vec4(aPos, 1.0);

  // Calculate TBN matrix for Tangent Space
  mat3 normalizer = transpose(inverse(mat3(model)));
  vec3 T = normalize(vec3(normalizer * aTangent));
  vec3 N = normalize(vec3(normalizer * aNormal));
  T = normalize(T - dot(T, N) * N);
  vec3 B = cross(N, T);

  // We want to do the lighting calculation in T-Space, 
  // so we need the inverse of the TBN. (orthonormal)
  mat3 TBN = transpose(mat3(T, B, N));

  // Send all relevant world data to the T-Space
  tspace.frag_pos   = TBN * vec3(model * vec4(aPos, 1.0));
  tspace.camera_pos = TBN * camera_pos;
  tspace.sun_dir    = TBN * sun_dir;
  tspace.light_pos  = TBN * light_pos;

  uv = aUV;
}

)";
const char *shader_fragment = R"(

#version 330 core
out vec4 FragColor;

in vec2 uv;
in vertex_output_tangent {
  
  vec3 frag_pos;
  vec3 camera_pos;
  vec3 sun_dir;
  vec3 light_pos;

} tspace;

uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform sampler2D normal_map;
uniform float shininess;

// 1 - use torch
uniform int torch = 1;

vec3 light_dir(vec3 N, vec3 V, vec3 direction, vec3 color) {
  
  vec3 L = normalize(-direction);
  vec3 R = reflect(-L, N);

  float kD = max(dot(L, N), 0.0f);
  float kS = pow(max(dot(V, R), 0.0f), shininess);

  vec3 ambient  = color * 0.1f * texture(diffuse_map, uv).rgb;
  vec3 diffuse  = color * kD   * texture(diffuse_map, uv).rgb;
  vec3 specular = color * kS   * texture(specular_map, uv).rgb;

  return ambient + diffuse + specular;
}

vec3 light_point(vec3 N, vec3 V, vec3 position, vec3 attenuation, vec3 color) {
  
  vec3 L = normalize(position - tspace.frag_pos);
  vec3 R = reflect(-L, N);

  float kD = max(dot(L, N), 0.0f);
  float kS = pow(max(dot(V, R), 0.0f), shininess);

  // Attenuazione
  float distance = length(position - tspace.frag_pos);
  float kFO = 1.0f / (attenuation.x + attenuation.y * distance 
      + attenuation.z * distance * distance);

  vec3 diffuse  = color * kFO * kD * texture(diffuse_map, uv).rgb;
  vec3 specular = color * kFO * kS * texture(specular_map, uv).rgb;

  return diffuse + specular;
}

vec3 light_spot(vec3 N, vec3 V, vec3 position, vec3 direction, vec2 cutoff, vec3 color) {

  vec3 L = normalize(position - tspace.frag_pos);
  vec3 R = reflect(-L, N);

  float kD = max(dot(L, N), 0.0f);
  float kS = pow(max(dot(V, R), 0.0f), shininess);

  float inner = cos(radians(cutoff.x));
  float outer = cos(radians(cutoff.y));

  // Cutoff
  float theta = dot(L, normalize(-direction));
  float epsilon = inner - outer;
  float kFO = clamp((theta - inner) / epsilon, 0.0f, 1.0f);

  vec3 diffuse  = color * kFO * kD * texture(diffuse_map, uv).rgb;
  vec3 specular = color * kFO * kS * texture(specular_map, uv).rgb;

  return diffuse + specular;
}

// All lighting is in tangent space.
void main() {
  vec3 result = vec3(0.0f);

  vec3 V = normalize(tspace.camera_pos - tspace.frag_pos);
  vec3 N = texture(normal_map, uv).rgb;
  N = normalize(N * 2.0 - 1.0);

  // Calculate contribution of all lights
  result += light_dir(N, V, tspace.sun_dir, vec3(0.15f, 0.15f, 0.3f));
  result += light_point(N, V, tspace.light_pos, vec3(1.0f, 0.01f, 0.032f), vec3(1.0f, 0.3f, 0.3f));
  result += light_spot(N, V, tspace.camera_pos, -tspace.camera_pos, vec2(6.5f, 8.5f), vec3(0.3f, 1.0f, 3.0f) * torch);

  FragColor = vec4(result, 1.0f);
}

)";

const char *shader_fragment_light = R"(
#version 330 core
out vec4 FragColor;
void main() {
  FragColor = vec4(1.0f);
}
)";

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

glib::camera_t camera = glib::camera_base(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

int main(int argc, char **argv) {

  GLFWwindow *window = glib::initialize(WIDTH, HEIGHT, "Window!");
  if (window == NULL) {
    return -1;
  }
  glViewport(0, 0, WIDTH, HEIGHT);

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  std::vector<float> vertices = glib::mesh_cube();
  glib::buffer_t cube =
      glib::buffer_create(&vertices, NULL, glib::basic_layout);

  // Load model of backpack
  glib::model_t backpack = glib::model_load(
      "/home/z1ko/develop/glexercises/data/models/backpack/backpack.obj");

  glib::program_t program =
      glib::program_create(shader_vertex, shader_fragment);
  glib::program_t program_light =
      glib::program_create(shader_vertex_light, shader_fragment_light);

  const float FOV = 45.0f;
  const float ASPECT_RATIO = (float)WIDTH / HEIGHT;
  const float ORBIT_DISTANCE = 10.0f;

  glm::vec3 sun_pos = glm::vec3(10.0f, 10.0f, 10.0f);
  glm::vec3 sun_dir = glm::normalize(-sun_pos);

  float deltaTime = 0.0f;
  float lastFrame = 0.0f;

  glEnable(GL_DEPTH_TEST);
  while (!glfwWindowShouldClose(window)) {

    float currentTime = glfwGetTime();
    deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;

    glib::input_handle_standard(window);

    static int torch = 1;
    static bool t_pressed = false;
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !t_pressed) {
      t_pressed = true;
      torch = !torch;
      glib::program_uniform_1i(program, "torch", torch);
    }

    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE)
      t_pressed = false;

    // Update orbiting point light position
    glm::vec3 point_light_pos;
    point_light_pos.z = sinf(glfwGetTime()) * 3.0f;
    point_light_pos.y = cosf(glfwGetTime()) * 3.0f;

    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set camera position
    camera.position.x = sinf(glfwGetTime()) * ORBIT_DISTANCE;
    camera.position.z = cosf(glfwGetTime()) * ORBIT_DISTANCE;
    glib::camera_look_at(camera, glm::vec3(0.0f));
    glib::program_uniform_3f(program, "camera_pos", camera.position.x,
                             camera.position.y, camera.position.z);

    // View matrix
    glm::mat4 view = glib::camera_view(camera);
    glib::program_uniform_mf(program, "view", glm::value_ptr(view));
    glib::program_uniform_mf(program_light, "view", glm::value_ptr(view));

    // Projection matrix
    glm::mat4 proj = glm::mat4(1.0f);
    proj = glm::perspective(glm::radians(FOV), ASPECT_RATIO, 0.1f, 100.0f);
    glib::program_uniform_mf(program, "proj", glm::value_ptr(proj));
    glib::program_uniform_mf(program_light, "proj", glm::value_ptr(proj));

    // Set material of normal cube
    glib::program_uniform_1i(program, "diffuse_map", 0);  // sampler
    glib::program_uniform_1i(program, "specular_map", 1); // sampler
    glib::program_uniform_1i(program, "normal_map", 2);   // sampler
    glib::program_uniform_1f(program, "shininess", 64.0f);

    // Set sunlight properties
    glib::program_uniform_3f(program, "sun_dir", sun_dir.x, sun_dir.y,
                             sun_dir.z);

    // Set point light position
    glib::program_uniform_3f(program, "light_pos", point_light_pos.x,
                             point_light_pos.y, point_light_pos.z);

    // Render backpack
    {
      glm::mat4 model = glm::mat4(1.0f);
      // model = glm::scale(model, glm::vec3(0.5f));

      glib::program_uniform_mf(program, "model", glm::value_ptr(model));
      glib::model_render(backpack, program);
    }

    // Render point light
    {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, point_light_pos);
      model = glm::scale(model, glm::vec3(0.2f));

      glib::program_uniform_mf(program_light, "model", glm::value_ptr(model));
      glib::render(cube, program_light);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();
  return 0;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}
