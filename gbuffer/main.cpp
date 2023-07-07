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

#define GLIB_MODEL_IMPL
#include <model.hpp>

const int WIDTH = 1600;
const int HEIGHT = 900;

const char* shader_geometry_fs = R"(
#version 330 core

layout (location = 0) out vec3 g_position;
layout (location = 1) out vec3 g_normal;
layout (location = 2) out vec4 g_color_spec;

out vec4 FragCol;

in mat3 TBN;
in vec3 frag_pos;
in vec2 uv;

uniform struct {
  sampler2D diffuse;
  sampler2D specular;
  sampler2D normal;
} material;

void main() {
  
  FragCol = vec4(0.6, 0.6, 0.2, 1.0);
 
  // Get normal in world-space
  vec3 N = texture(material.normal, uv).xyz;
  N = normalize(N * 2.0 - 1.0);
  N = TBN * N;

  vec3 g_position = frag_pos;
  g_normal = N;
  vec4 g_color_spec = vec4(texture(material.diffuse, uv).rgb, texture(material.specular, uv).r);
}
)";

const char* shader_geometry_vs = R"(
#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec3 a_tangent;
layout (location = 3) in vec2 a_uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out mat3 TBN;
out vec3 frag_pos;
out vec2 uv;

void main() {
  gl_Position = proj * view * model * vec4(a_position, 1.0);

  // Calculate TBN matrix for tangent space
  mat3 nmodel = transpose(inverse(mat3(model)));
  vec3 T = normalize(nmodel * a_tangent);
  vec3 N = normalize(nmodel * a_normal);
  T = normalize(T - cross(T, N) * N);
  vec3 B = cross(N, T);

  TBN = mat3(T, B, N);
  frag_pos = vec3(model * vec4(a_position, 1.0));
  uv = a_uv;
}
)";

const char* shader_vertex_light = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {
  gl_Position = proj * view * model * vec4(aPos, 1.0);
}

)";

const char* shader_fragment_light = R"(
#version 330 core
out vec4 FragColor;
void main() {
  FragColor = vec4(1.0f);
}
)";

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

glib::camera_t camera = glib::camera_base(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = WIDTH  / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

int main(int argc, char** argv) {
  GLFWwindow* window = glib::initialize(WIDTH, HEIGHT, "Window!");
  if (window == NULL) { return -1; }
  glViewport(0, 0, WIDTH, HEIGHT);
    
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // Programs for deferred rendering
  glib::program_t program_geometry = glib::program_create(shader_geometry_vs, shader_geometry_fs);
  //glib::program_t program_lighting = glib::program_load("../lighting.shader");

  std::vector<float> cube_vertices = glib::mesh_cube();
  glib::buffer_t cube = glib::buffer_create(&cube_vertices, NULL, glib::basic_layout);
  glib::program_t program_light = glib::program_create(shader_vertex_light, shader_fragment_light);

  printf("A\n");

  // Load model of backpack
  glib::model_t backpack = glib::model_load("/home/z1ko/univr/graphics/data/models/backpack/backpack.obj");
  
  const float FOV = 45.0f;
  const float ASPECT_RATIO = (float)WIDTH/HEIGHT;

  float deltaTime = 0.0;
  float lastFrame = 0.0;

  glEnable(GL_DEPTH_TEST);
  while(!glfwWindowShouldClose(window)) {

    float currentTime = glfwGetTime();
    deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;

    glib::input_handle_standard(window);
    glib::camera_input(camera, window, deltaTime);

    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set camera position
    glib::camera_look_at(camera, glm::vec3(0.0f));
    //glib::program_uniform_3f(program, "camera_pos", 
    //    camera.position.x, camera.position.y, camera.position.z);

    // View matrix
    glm::mat4 view = glib::camera_view(camera);
    glib::program_uniform_mf(program_geometry, "view", glm::value_ptr(view));
    glib::program_uniform_mf(program_light, "view", glm::value_ptr(view));

    // Projection matrix
    glm::mat4 proj = glm::mat4(1.0f);
    proj = glm::perspective(glm::radians(FOV), ASPECT_RATIO, 0.1f, 100.0f);
    glib::program_uniform_mf(program_geometry, "proj", glm::value_ptr(proj));
    glib::program_uniform_mf(program_light, "proj", glm::value_ptr(proj));

    // Set attached texture slot to samplers
    glib::program_uniform_1i(program_geometry, "diffuse_map",  0); // sampler
    glib::program_uniform_1i(program_geometry, "specular_map", 1); // sampler
    glib::program_uniform_1i(program_geometry, "normal_map",   2); // sampler

    // Render all backpacks
    {
      const int side_count = 5;
      const float side_lenght = 30.0;
      const float delta = side_lenght / side_count;

      glm::mat4 model;
      for (int i = 0; i < side_count; ++i) {
        for (int j = 0; j < side_count; ++j) {
          float x = delta * i - side_lenght * 0.5 + delta * 0.5;
          float z = delta * j - side_lenght * 0.5 + delta * 0.5;

          model = glm::mat4(1.0);
          model = glm::translate(model, glm::vec3(x, 0.0, z));
          glib::program_uniform_mf(program_geometry, "model", glm::value_ptr(model));
          glib::model_render(backpack, program_geometry);
        }
      }
    }

    /*
    // Render point light
    {
      glm::mat4 model = glm::mat4(1.0f);
      model = glm::translate(model, point_light_pos);
      model = glm::scale(model, glm::vec3(0.2f));
      
      glib::program_uniform_mf(program_light, "model", glm::value_ptr(model));
      glib::render(cube, program_light);
    }
    */

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  
  glfwTerminate();
  return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

