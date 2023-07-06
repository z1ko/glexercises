#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace glib {

const glm::vec3 UP(0.0f, 1.0f, 0.0f);

struct camera_t {

  glm::vec3 position;
  glm::vec3 front;
  glm::vec3 right;
  glm::vec3 up;

  float yaw, pitch;
};

camera_t camera_base(const glm::vec3 &position);

void camera_input(camera_t &camera, GLFWwindow *window, float delta, bool fps = false);
void camera_input_mouse(camera_t &camera, float xoffset, float yoffset);

glm::mat4 camera_look_at(camera_t &camera, const glm::vec3 &target);
glm::mat4 camera_view(const camera_t &camera);

#ifdef GLIB_TRANSFORM_IMPL
#undef GLIB_TRANSFORM_IMPL

camera_t camera_base(const glm::vec3 &position) {
  camera_t result;
  result.position = position;
  result.yaw = -90.0f;
  result.pitch = 0.0f;

  result.front = glm::vec3(0.0f, 0.0f, -1.0f);
  result.right = glm::normalize(glm::cross(result.front, UP));
  result.up    = glm::normalize(glm::cross(result.right, result.front));

  //result.view = glm::lookAt(result.position, result.position + result.front, result.up);
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
  if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    camera_input_mouse(camera, -5.0f, 0.0f);
  if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    camera_input_mouse(camera,  5.0f, 0.0f);
  if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    camera.position += cameraSpeed * camera.up;
  if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    camera.position -= cameraSpeed * camera.up;

  if (fps)
    camera.position.y = 0.0f;
  
  //camera.view = glm::lookAt(camera.position, camera.position + camera.front, camera.up);
}

void camera_input_mouse(camera_t &camera, float xoffset, float yoffset) {
  const float SENSITIVITY = 0.01f;

  camera.yaw   += xoffset * SENSITIVITY;
  camera.pitch += yoffset * SENSITIVITY;

  if (camera.pitch >  89.0f) camera.pitch =  89.0f;
  if (camera.pitch < -89.0f) camera.pitch = -89.0f;

  glm::vec3 front;
  front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
  front.y = sin(glm::radians(camera.pitch));
  front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));

  camera.front = glm::normalize(front);
  camera.right = glm::normalize(glm::cross(camera.front, UP));
  camera.up    = glm::normalize(glm::cross(camera.right, camera.front));

  //camera.view = glm::lookAt(camera.position, camera.position + camera.front, camera.up);
}

glm::mat4 camera_look_at(camera_t &camera, const glm::vec3 &target) {

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

  camera.front = glm::normalize(target - camera.position);
  camera.right = glm::normalize(glm::cross(camera.front, UP));
  camera.up    = glm::normalize(glm::cross(camera.right, camera.front));

  return rotation * translation;
}

glm::mat4 camera_view(const camera_t &camera) {
  return glm::lookAt(camera.position, camera.position + camera.front, camera.up);
}

#endif

} // namespace glib
