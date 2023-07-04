#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace glib {

// Create new window
GLFWwindow* initialize(int width, int height, const char* name);

#ifdef GLIB_INIT_IMPL
#undef GLIB_INIT_IMPL

GLFWwindow* initialize(int width, int height, const char* name) {
  if (!glfwInit())
    return NULL;
  
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(width, height, name, NULL, NULL);
  if (window == NULL) {
    std::cout << "Couldn't create window\n";
    glfwTerminate();
    return NULL;
  }

  glfwMakeContextCurrent(window);
  if (!gladLoadGLLoader(((GLADloadproc)glfwGetProcAddress))) {
    std::cout << "Couldn't load opengl\n";
    glfwTerminate();
    return NULL;
  }

  return window;
}

#endif

} // namespace glib
