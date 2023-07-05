#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace glib {

struct camera_t {
  glm::mat4 view;

  glm::vec3 position;
  glm::vec3 front;

  float yaw, pitch;
};

camera_t camera_base(const glm::vec3 &position);

void camera_input(camera_t &camera, GLFWwindow *window, float delta, bool fps = false);
void camera_look_at(camera_t &camera, const glm::vec3 &target);

#ifdef GLIB_TRANSFORM_IMPL
#undef GLIB_TRANSFORM_IMPL

camera_t camera_base(const glm::vec3 &position) {
  camera_t result;
  result.position = position;
  result.front = glm::vec3(0.0f, 0.0f, -1.0f);
  result.yaw = 0.0f;
  result.pitch = 0.0f;
  result.view = glm::lookAt(result.position, result.position + result.front, glm::vec3(0.0f, 1.0f, 0.0f));
  return result;
}

void camera_input(camera_t &camera, GLFWwindow *window, float delta, bool fps) {
  const glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

  // Movement
  const float cameraSpeed = 2.5f * delta;
  if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
      camera.position += cameraSpeed * camera.front;
  if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
      camera.position -= cameraSpeed * camera.front;
  if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
      camera.position -= glm::normalize(glm::cross(camera.front, cameraUp)) * cameraSpeed;
  if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
      camera.position += glm::normalize(glm::cross(camera.front, cameraUp)) * cameraSpeed;

  // Mouse
  /*
  static double last_x, last_y;
  static bool mouse_initialized = false;
  if (!mouse_initialized) {
    mouse_initialized = true;
    glfwGetCursorPos(window, &last_x, &last_y); 
  }

  double x, y;
  glfwGetCursorPos(window, &x, &y); 

  float xoffset = x - last_x;
  float yoffset = last_y - y;

  const float cameraSensitivity = 0.1f;
  camera.yaw   += xoffset * cameraSensitivity;
  camera.pitch += yoffset * cameraSensitivity;

  if (camera.pitch >  89.0f) camera.pitch =  89.0f;
  if (camera.pitch < -89.0f) camera.pitch = -89.0f;

  // Calculate front of camera
  glm::vec3 direction(
    cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch)),
    sin(glm::radians(camera.pitch)),
    sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch))
  );
  
  camera.front = glm::normalize(direction);
  
  */

  if (fps)
    camera.position.y = 0.0f;

  camera.view = glm::lookAt(camera.position, camera.position + camera.front, glm::vec3(0.0f, 1.0f, 0.0f));
}

void camera_look_at(camera_t &camera, const glm::vec3 &target) {

  glm::vec3 zax = glm::normalize(camera.position - target);
  glm::vec3 xax = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), zax));
  glm::vec3 yax = glm::cross(zax, xax);

  glm::mat4 translation = glm::mat4(1.0f);
  translation = glm::translate(translation, -camera.position);

  glm::mat4 rotation = glm::mat4(1.0f);
  rotation[0][0] = xax.x;
  rotation[1][0] = xax.y;
  rotation[2][0] = xax.z;
  rotation[0][1] = yax.x;
  rotation[1][1] = yax.y;
  rotation[2][1] = yax.z;
  rotation[0][2] = zax.x;
  rotation[1][2] = zax.y;
  rotation[2][2] = zax.z;

  camera.view = rotation * translation;
  camera.front = glm::normalize(target - camera.position);
}

#endif

} // namespace glib
