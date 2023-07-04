#pragma once

#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>

namespace glib {

struct vertex_t {
  float x, y, z;
};

using index_t = unsigned int;

// How to draw the buffer
enum buffer_draw_e 
{
  GLIB_DRAW_ARRAYS,
  GLIB_DRAW_ELEMENTS
};

struct buffer_t {
  buffer_draw_e draw;
  unsigned int vao, vbo, ebo;
  unsigned int v_count, e_count;
};

struct program_t {
  unsigned int id;
};

// Create a VAO using a VBO and a EBO
buffer_t buffer_create(vertex_t *v, unsigned int v_count, index_t *e, unsigned int e_count);
#define buffer_bind(buffer) glBindVertexArray(buffer.vao)
#define buffer_unbind() glBindVertexArray(0)

// Create program from vertex and fragment source
program_t program_create(const char* vertex, const char* fragment);
#define program_bind(program) glUseProgram(program.id)
#define program_unbind() glUseProgram(0)

// Render a buffer with a program
void render(const buffer_t &buffer, const program_t &program);

#ifdef GLIB_GRAPHICS_IMPL
#undef GLIB_GRAPHICS_IMPL

buffer_t buffer_create(vertex_t *v, unsigned int v_count, index_t *e, unsigned int e_count) {
  
  buffer_t result = { };
  result.v_count = v_count;
  result.e_count = e_count;

  // How to draw the element
  result.draw = GLIB_DRAW_ARRAYS;
  if (e != NULL)
    result.draw = GLIB_DRAW_ELEMENTS;

  glGenVertexArrays(1, &result.vao);
  glBindVertexArray(result.vao);
  {
    glGenBuffers(1, &result.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
    {
      // Set buffer data
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_t) * v_count, v, GL_STATIC_DRAW);
      // Set buffer layout
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
      glEnableVertexAttribArray(0);
    }

    // Add elements buffer 
    if (e != NULL) {
      glGenBuffers(1, &result.ebo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);
      {
        // Set buffer data
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_t) * e_count, e, GL_STATIC_DRAW);
      }
    }
  }
  glBindVertexArray(0);

  printf("created buffer(vao: %d, vbo: %d, ebo: %d)\n", result.vao, result.vbo, result.ebo);
  return result;
}

static inline bool check_shader_compilation(unsigned int id) {

  int success;
  glGetShaderiv(id, GL_COMPILE_STATUS, &success);
  if(!success) {
    char infoLog[512];
    glGetShaderInfoLog(id, 512, NULL, infoLog);
    std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    exit(1);
  }

  return success;
}

static inline bool 
check_shader_linking(unsigned int id) {

  int success;
  glGetProgramiv(id, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(id, 512, NULL, infoLog);
    std::cout << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    exit(1);
  }

  return success;
}

program_t program_create(const char* vertex, const char* fragment) {
  
  unsigned int vid = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vid, 1, &vertex, NULL);

  glCompileShader(vid);
  check_shader_compilation(vid);

  unsigned int fid = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fid, 1, &fragment, NULL);

  glCompileShader(fid);
  check_shader_compilation(fid);

  unsigned int sid = glCreateProgram();
  glAttachShader(sid, vid);
  glAttachShader(sid, fid);
  glLinkProgram(sid);

  glDeleteShader(vid);
  glDeleteShader(fid);

  return {
    .id = sid
  };
}

void render(const buffer_t &buffer, const program_t &program) {
  program_bind(program);
  buffer_bind(buffer);

  switch (buffer.draw) {
    case GLIB_DRAW_ARRAYS:
      glDrawArrays(GL_TRIANGLES, 0, buffer.v_count);
      break;
    case GLIB_DRAW_ELEMENTS:
      glDrawElements(GL_TRIANGLES, buffer.e_count, GL_UNSIGNED_INT, 0);
      break;
    default:
      printf("Unknown draw mode!\n");  
      break;
  }

  buffer_unbind();
  program_unbind();
}

#endif

} // namespace glib