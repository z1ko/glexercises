
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

const std::vector<glm::vec3> cube_positions = {
  glm::vec3(  0.0f,  0.0f,   0.0f ),
  glm::vec3(  2.0f,  5.0f, -15.0f ),
  glm::vec3( -1.5f, -2.2f,  -2.5f ),
  glm::vec3( -3.8f, -2.0f, -12.3f ),
  glm::vec3(  2.4f, -0.4f,  -3.5f ),
  glm::vec3( -1.7f,  3.0f,  -7.5f ),
  glm::vec3(  1.3f, -2.0f,  -2.5f ),
  glm::vec3(  1.5f,  2.0f,  -2.5f ),
  glm::vec3(  1.5f,  0.2f,  -1.5f ),
  glm::vec3( -1.3f,  1.0f,  -1.5f )
};

const int WIDTH = 1600;
const int HEIGHT = 900;

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

const char* shader_vertex = R"(

#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec2 aUV;

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
const char* shader_fragment = R"(

#version 330 core
out vec4 FragColor;

in vec3 frag_pos;
in vec3 normal;
in vec2 uv;

uniform vec3 cameraPos;

uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform sampler2D emission_map;
uniform float shininess;

uniform struct {
  vec3 direction;
  vec3 color;
} sun;

struct point_light_t {
  vec3 position;
  vec3 color;
  
  // Falloff
  float constant;
  float linear;
  float quadratic;
};

struct spot_light_t {
  vec3 position;
  vec3 direction;
  vec3 color;

  float cutoff_inner;
  float cutoff_outer;
};

#define POINT_LIGHT_CAP 10
uniform point_light_t point_light_list[POINT_LIGHT_CAP];
uniform int point_light_count = 0;

#define SPOT_LIGHT_CAP 10
uniform spot_light_t spot_light_list[SPOT_LIGHT_CAP];
uniform int spot_light_count = 0;

vec3 light_sun(vec3 N, vec3 V) {
  
  vec3 L = normalize(-sun.direction);
  vec3 R = reflect(-L, N);

  float kD = max(dot(L, N), 0.0f);
  float kS = pow(max(dot(V, R), 0.0f), shininess);

  vec3 ambient  = sun.color * 0.1f * texture(diffuse_map, uv).rgb;
  vec3 diffuse  = sun.color * kD   * texture(diffuse_map, uv).rgb;
  vec3 specular = sun.color * kS   * texture(specular_map, uv).rgb;

  return ambient + diffuse + specular;
}

vec3 light_point(vec3 N, vec3 V, int i) {
  point_light_t light = point_light_list[i];
  
  vec3 L = normalize(light.position - frag_pos);
  vec3 R = reflect(-L, N);

  float kD = max(dot(L, N), 0.0f);
  float kS = pow(max(dot(V, R), 0.0f), shininess);

  // Attenuazione
  float distance = length(light.position - frag_pos);
  float kFO = 1.0f / (light.constant + light.linear * distance 
      + light.quadratic * distance * distance);

  vec3 diffuse  = light.color * kFO * kD * texture(diffuse_map, uv).rgb;
  vec3 specular = light.color * kFO * kS * texture(specular_map, uv).rgb;

  return diffuse + specular;
}

vec3 light_spot(vec3 N, vec3 V, int i) {
  spot_light_t light = spot_light_list[i];

  vec3 L = normalize(light.position - frag_pos);
  vec3 R = reflect(-L, N);

  float kD = max(dot(L, N), 0.0f);
  float kS = pow(max(dot(V, R), 0.0f), shininess);

  // Cutoff
  float theta = dot(L, normalize(-light.direction));
  float epsilon = light.cutoff_inner - light.cutoff_outer;
  float kFO = clamp((theta - light.cutoff_outer) / epsilon, 0.0f, 1.0f);

  vec3 diffuse  = light.color * kFO * kD * texture(diffuse_map, uv).rgb;
  vec3 specular = light.color * kFO * kS * texture(specular_map, uv).rgb;

  return diffuse + specular;
}

void main() {
  vec3 result = vec3(0.0f);
  
  vec3 N = normalize(normal);
  vec3 V = normalize(cameraPos - frag_pos);

  // Apply sun light
  result += light_sun(N, V);

  // Apply all point lights
  for (int i = 0; i < point_light_count; i++) {
    result += light_point(N, V, i);
  }

  // Apply all spot lights
  for (int i = 0; i < spot_light_count; i++) {
    result += light_spot(N, V, i);
  }

  FragColor = vec4(result, 1.0f);
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
  
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  std::vector<float> vertices = glib::mesh_cube();
  glib::buffer_t cube = glib::buffer_create(&vertices, NULL, glib::basic_layout);

  // Load model of backpack
  glib::model_t backpack = glib::model_load("/home/z1ko/univr/graphics/data/models/backpack/backpack.obj");

  glib::program_t program = glib::program_create(shader_vertex, shader_fragment);
  glib::program_t program_light = glib::program_create(shader_vertex_light, shader_fragment_light);

  const float FOV = 45.0f;
  const float ASPECT_RATIO = (float)WIDTH/HEIGHT;

  glm::vec3 sun_pos = glm::vec3(10.0f, 10.0f, 10.0f);
  glm::vec3 sun_dir = glm::normalize(-sun_pos);

  float deltaTime = 0.0f;
  float lastFrame = 0.0f;

  glEnable(GL_DEPTH_TEST);
  while(!glfwWindowShouldClose(window)) {

    float currentTime = glfwGetTime();
    deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;

    glib::input_handle_standard(window);
    glib::camera_input(camera, window, deltaTime);

    // Reset camera
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
      glib::camera_look_at(camera, glm::vec3(0.0f));
      firstMouse = true;
    }

    // Update orbiting point light position
    glm::vec3 point_light_pos;
    point_light_pos.x = sinf(glfwGetTime()) * 3.0f;
    point_light_pos.z = cosf(glfwGetTime()) * 3.0f;

    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Set camera position for light calculation
    glib::program_uniform_3f(program, "cameraPos", 
        camera.position.x, camera.position.y, camera.position.z);

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
    glib::program_uniform_1i(program, "diffuse_map",  0); // sampler
    glib::program_uniform_1i(program, "specular_map", 1); // sampler
    glib::program_uniform_1i(program, "emission_map", 2); // sampler
    glib::program_uniform_1f(program, "shininess", 64.0f);

    // Set sunlight properties
    glib::program_uniform_3f(program, "sun.color", 0.3f, 0.3f, 0.7f);
    glib::program_uniform_3f(program, "sun.direction", 
      sun_dir.x, sun_dir.y, sun_dir.z);

    // Set pointlight properties
    glib::program_uniform_1i(program, "point_light_count", 1);
    glib::program_uniform_1f(program, "point_light_list[0].constant",  1.0f);
    glib::program_uniform_1f(program, "point_light_list[0].linear",    0.09f);
    glib::program_uniform_1f(program, "point_light_list[0].quadratic", 0.032f);
    glib::program_uniform_3f(program, "point_light_list[0].color", 1.0f, 0.3f, 0.3f);
    glib::program_uniform_3f(program, "point_light_list[0].position", 
        point_light_pos.x, point_light_pos.y, point_light_pos.z);

    // Set spotlight properties
    glib::program_uniform_1i(program, "spot_light_count", 1);
    glib::program_uniform_1f(program, "spot_light_list[0].cutoff_inner", glm::cos(glm::radians(12.5f)));
    glib::program_uniform_1f(program, "spot_light_list[0].cutoff_outer", glm::cos(glm::radians(17.5f)));
    glib::program_uniform_3f(program, "spot_light_list[0].color", 0.3f, 1.0f, 0.3f);
    glib::program_uniform_3f(program, "spot_light_list[0].position", 
        camera.position.x, camera.position.y, camera.position.z);
    glib::program_uniform_3f(program, "spot_light_list[0].direction", 
        camera.front.x, camera.front.y, camera.front.z);
    
    // Render backpack
    {
      glm::mat4 model = glm::mat4(1.0f);
      //model = glm::scale(model, glm::vec3(0.5f));

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

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

