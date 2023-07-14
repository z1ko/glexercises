#include <cmath>
#include <cstdio>
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

#define GLIB_GBUFFER_IMPL
#include <gbuffer.hpp>

const int WIDTH = 1600;
const int HEIGHT = 900;
const int LIGHT_COUNT = 128;

const char *shader_geometry_fs = R"(
#version 330 core

layout (location = 0) out vec3 g_position;
layout (location = 1) out vec3 g_normal;
layout (location = 2) out vec4 g_color_spec;

//out vec4 FragCol;

in mat3 TBN;
in vec3 frag_pos;
in vec2 uv;

uniform struct {
  sampler2D diffuse;
  sampler2D specular;
  sampler2D normal;
} material;

#define COMPONENT 3

void main() {
  
  // Get normal in world-space
  vec3 N = texture(material.normal, uv).xyz;
  N = normalize(N * 2.0 - 1.0);
  N = TBN * N;

#if COMPONENT == 0
  g_position = frag_pos;
#elif COMPONENT == 1
  g_position = N;
#elif COMPONENT == 2
  g_position = texture(material.diffuse, uv).rgb;
#else
  g_position = vec3(texture(material.specular, uv).r);
#endif

  g_normal = N;
  g_color_spec = vec4(texture(material.diffuse, uv).rgb, 
    texture(material.specular, uv).r);
}
)";

const char *shader_geometry_vs = R"(
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

const char *shader_lighting_fs = R"(
#version 330 core
out vec4 FragCol;

in vec2 uv;

// Output of geometry pass
uniform struct {
  sampler2D position;
  sampler2D normal;
  sampler2D color_spec;
} gbuffer;

struct light_point_t {
  vec3 position;
  vec3 color;
  vec3 attenuation;
}; 

#define LIGHT_CAPACITY 128
uniform light_point_t lights[LIGHT_CAPACITY];
uniform vec3 camera_pos;

#define AMBIENT 0.1f
void main() {
 
  vec3 P = texture(gbuffer.position,   uv).rgb;
  vec3 N = texture(gbuffer.normal,     uv).rgb;
  vec4 C = texture(gbuffer.color_spec, uv);

  vec3 color = C.rgb;
  float spec = C.a;

  vec3 V = normalize(camera_pos - P);

  vec3 result = color * AMBIENT;
  for (int i = 0; i < LIGHT_CAPACITY; ++i) {
    light_point_t light = lights[i];

    vec3 L = normalize(light.position - P);
    vec3 R = reflect(-L, N);

    float kD = max(dot(L, N), 0.0f);
    float kS = pow(max(dot(R, V), 0.0f), 64.0f);

    // Attenuation
    float distance = length(light.position - P);
    float kA = 1.0 / (light.attenuation.x + light.attenuation.y * distance 
      + light.attenuation.z * distance * distance);

    result += color * kD * kA * light.color;
    //result += spec  * kS * kA * light.color;
  }
  
  FragCol = vec4(result, 1.0f);
}
)";

const char *shader_lighting_vs = R"(
#version 330 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;

out vec2 uv;

void main() {
  gl_Position = vec4(a_position, 1.0);
  uv = a_uv;
}
)";

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

const char *shader_fragment_light = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 color;

void main() {
  FragColor = vec4(color, 1.0);
}
)";

struct light_t {
  glm::vec3 position;
  glm::vec3 color;
};

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void generate_lights(std::vector<light_t> &lights);

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

  // Programs for deferred rendering
  glib::program_t program_geometry =
      glib::program_create(shader_geometry_vs, shader_geometry_fs);
  glib::program_t program_lighting =
      glib::program_create(shader_lighting_vs, shader_lighting_fs);

  std::vector<float> cube_vertices = glib::mesh_cube();
  glib::buffer_t cube =
      glib::buffer_create(&cube_vertices, NULL, glib::basic_layout);
  glib::program_t program_light =
      glib::program_create(shader_vertex_light, shader_fragment_light);

  // Set attached texture slot to samplers of geometry pass
  glib::program_uniform_1i(program_geometry, "material.diffuse", 0);
  glib::program_uniform_1i(program_geometry, "material.specular", 1);
  glib::program_uniform_1i(program_geometry, "material.normal", 2);

  // Set attached texture slot to samplers of Lighting pass
  glib::program_uniform_1i(program_lighting, "gbuffer.position", 0);
  glib::program_uniform_1i(program_lighting, "gbuffer.normal", 1);
  glib::program_uniform_1i(program_lighting, "gbuffer.color_spec", 2);

  // Load model of backpack
  glib::model_t backpack =
      glib::model_load("../../data/models/backpack/backpack.obj");
  std::vector<glm::vec3> positions;
  positions.push_back(glm::vec3(-3.0, -0.5, -3.0));
  positions.push_back(glm::vec3(0.0, -0.5, -3.0));
  positions.push_back(glm::vec3(3.0, -0.5, -3.0));
  positions.push_back(glm::vec3(-3.0, -0.5, 0.0));
  positions.push_back(glm::vec3(0.0, -0.5, 0.0));
  positions.push_back(glm::vec3(3.0, -0.5, 0.0));
  positions.push_back(glm::vec3(-3.0, -0.5, 3.0));
  positions.push_back(glm::vec3(0.0, -0.5, 3.0));
  positions.push_back(glm::vec3(3.0, -0.5, 3.0));

  // Screen covering all screen in NDC-space
  std::vector<float> screen_vertices = glib::mesh_screen_ndc();
  glib::buffer_t screen =
      glib::buffer_create(&screen_vertices, NULL, glib::layout_3F2F);

  const float FOV = 45.0f;
  const float ASPECT_RATIO = (float)WIDTH / HEIGHT;

  float deltaTime = 0.0;
  float lastFrame = 0.0;

  // Where to store the output of the geometry pass
  glib::gbuffer_t gbuffer = glib::gbuffer_create(WIDTH, HEIGHT);

  // All lights of the scene
  std::vector<light_t> lights;
  generate_lights(lights);

  glEnable(GL_DEPTH_TEST);
  while (!glfwWindowShouldClose(window)) {

    // Regenerate lights
    static bool pressed = false;
    switch (glfwGetKey(window, GLFW_KEY_G)) {
    case GLFW_PRESS:
      if (!pressed) {
        pressed = true;
        generate_lights(lights);
      }
      break;
    case GLFW_RELEASE:
      pressed = false;
      break;
    }

    float currentTime = glfwGetTime();
    deltaTime = currentTime - lastFrame;
    lastFrame = currentTime;

    glib::input_handle_standard(window);
    glib::camera_input(camera, window, deltaTime);
    glib::camera_look_at(camera, glm::vec3(0.0f));

    // View matrix
    glm::mat4 view = glib::camera_view(camera);
    glib::program_uniform_mf(program_geometry, "view", glm::value_ptr(view));
    glib::program_uniform_mf(program_light, "view", glm::value_ptr(view));

    // Projection matrix
    glm::mat4 proj = glm::mat4(1.0f);
    proj = glm::perspective(glm::radians(FOV), ASPECT_RATIO, 0.1f, 100.0f);
    glib::program_uniform_mf(program_geometry, "proj", glm::value_ptr(proj));
    glib::program_uniform_mf(program_light, "proj", glm::value_ptr(proj));

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render all backpacks in gbuffer
    gbuffer_bind(gbuffer);
    {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      glm::mat4 model;
      for (glm::vec3 &position : positions) {

        model = glm::mat4(1.0);
        model = glm::translate(model, position);
        glib::program_uniform_mf(program_geometry, "model",
                                 glm::value_ptr(model));
        glib::model_render(backpack, program_geometry);
      }
    }
    gbuffer_unbind();

    // Lighting pass
    {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      // Bind all gbuffer textures
      glib::texture_bind(gbuffer.position, 0);
      glib::texture_bind(gbuffer.normal, 1);
      glib::texture_bind(gbuffer.color, 2);

      glib::program_uniform_3f(program_lighting, "camera_pos",
                               camera.position.x, camera.position.y,
                               camera.position.z);

      // Set al lights
      char uniform[64];
      int i = 0;
      for (light_t &light : lights) {

        snprintf(uniform, 64, "lights[%d].position", i);
        glib::program_uniform_3f(program_lighting, uniform, light.position.x,
                                 light.position.y, light.position.z);

        snprintf(uniform, 64, "lights[%d].color", i);
        glib::program_uniform_3f(program_lighting, uniform, light.color.x,
                                 light.color.y, light.color.z);

        snprintf(uniform, 64, "lights[%d].attenuation", i);
        glib::program_uniform_3f(program_lighting, uniform, 1.0, 0.7, 1.8);
        i += 1;
      }

      glib::render(screen, program_lighting, GL_TRIANGLE_STRIP);
    }

#if 1
    // Blit result of geometry pass into screen
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer.id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, WIDTH, HEIGHT, 0, 0, WIDTH, HEIGHT,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#else
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer.id);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, WIDTH, HEIGHT, 0, 0, WIDTH, HEIGHT,
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Render point light
    {
      for (light_t &light : lights) {
        glm::mat4 model = glm::mat3(1.0f);
        model = glm::translate(model, light.position);
        model = glm::scale(model, glm::vec3(0.2f));

        glib::program_uniform_mf(program_light, "model", glm::value_ptr(model));
        glib::program_uniform_3f(program_light, "color", light.color.x,
                                 light.color.y, light.color.z);

        glib::render(cube, program_light);
      }
    }

#endif
    glfwSwapBuffers(window);
    glfwPollEvents();
  }
  glfwTerminate();
  return 0;
}

void generate_lights(std::vector<light_t> &lights) {

  srand(time(0));
  lights.clear();
  for (int i = 0; i < LIGHT_COUNT; ++i) {
    float xPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
    float yPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 4.0);
    float zPos = static_cast<float>(((rand() % 100) / 100.0) * 6.0 - 3.0);
    float rColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5);
    float gColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5);
    float bColor = static_cast<float>(((rand() % 100) / 200.0f) + 0.5);
    lights.push_back({.position = glm::vec3(xPos, yPos, zPos),
                      .color = glm::vec3(rColor, gColor, bColor)});
  }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  glViewport(0, 0, width, height);
}
