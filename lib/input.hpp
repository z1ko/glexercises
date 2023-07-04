#pragma once

#include <GLFW/glfw3.h>

namespace glib {

// Handle generic 
void input_handle_standard(GLFWwindow* window);

#ifdef GLIB_INPUT_IMPL
#undef GLIB_INPUT_IMPL

void input_handle_standard(GLFWwindow *window) {
  if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

#endif

} // namespace glib
