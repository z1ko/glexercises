#pragma once

#include <GLFW/glfw3.h>
#include <iostream>
#include <cstdlib>
#include <functional>
#include <vector>
#include <cassert>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

namespace glib {

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

struct texture_t {
  unsigned int id;
};

// Basic position layout
std::function<void(void)> basic_layout = [](){
  // Position
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
};

// Basic position + color layout
std::function<void(void)> color_layout = [](){
  // Position
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // Color
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3* sizeof(float)));
  glEnableVertexAttribArray(1);
};

// Position + Color + UV layout
std::function<void(void)> texture_layout = []() {
  // Position
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  // Color
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3* sizeof(float)));
  glEnableVertexAttribArray(1);
  // UV
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
  glEnableVertexAttribArray(2);  
};

// Create a VAO using a VBO and a EBO, a lambda is used to determine the attributes layout
buffer_t buffer_create(std::vector<float> *data, std::vector<index_t> *indices, std::function<void(void)>& lambda = basic_layout);
#define buffer_bind(buffer) glBindVertexArray(buffer.vao)
#define buffer_unbind() glBindVertexArray(0)

// Create program from vertex and fragment source
program_t program_create(const char* vertex, const char* fragment);
#define program_bind(program) glUseProgram(program.id)
#define program_unbind() glUseProgram(0)

void program_uniform_1i(const program_t &program, const char* name, int value);
void program_uniform_1f(const program_t &program, const char* name, float value);
void program_uniform_3f(const program_t &program, const char* name, float x, float y, float z);
void program_uniform_mf(const program_t &program, const char* name, float *data);

texture_t texture_load(const char* path, unsigned int format, unsigned int wrapping);
void texture_bind(const texture_t &texture, int slot);
#define texture_unbind() glBindTexture(GL_TEXTURE_2D, 0);

// Render a buffer with a program
void render(const buffer_t &buffer, const program_t &program);

#ifdef GLIB_GRAPHICS_IMPL
#undef GLIB_GRAPHICS_IMPL

buffer_t buffer_create(std::vector<float> *data, std::vector<index_t> *indices, std::function<void(void)>& lambda) {
  assert(data && "Data must be provided!");

  buffer_t result = { };
  result.v_count = data->size();

  // How to draw the element
  result.draw = GLIB_DRAW_ARRAYS;
  if (indices != NULL) {
  result.e_count = indices->size();
    result.draw = GLIB_DRAW_ELEMENTS;
  }

  glGenVertexArrays(1, &result.vao);
  glBindVertexArray(result.vao);
  {
    glGenBuffers(1, &result.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, result.vbo);
    {
      // Set buffer data
      glBufferData(GL_ARRAY_BUFFER, sizeof(float) * result.v_count, data->data(), GL_STATIC_DRAW);
      // Set buffer layout
      lambda();

      /*
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
      glEnableVertexAttribArray(0);
      */
    }

    // Add elements buffer 
    if (indices != NULL) {
      glGenBuffers(1, &result.ebo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.ebo);
      {
        // Set buffer data
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(index_t) * result.e_count, indices->data(), GL_STATIC_DRAW);
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

void program_uniform_1i(const program_t &program, const char* name, int value) {
  program_bind(program);
  glUniform1i(glGetUniformLocation(program.id, name), value);
  program_unbind();
}

void program_uniform_1f(const program_t &program, const char* name, float value) {
  program_bind(program);
  glUniform1f(glGetUniformLocation(program.id, name), value);
  program_unbind();
}

void program_uniform_3f(const program_t &program, const char* name, float x, float y, float z) {
  program_bind(program);
  glUniform3f(glGetUniformLocation(program.id, name), x, y, z);
  program_unbind();
}

void program_uniform_mf(const program_t &program, const char* name, float *data) {
  program_bind(program);
  glUniformMatrix4fv(glGetUniformLocation(program.id, name), 1, GL_FALSE, data);
  program_unbind();
}

texture_t texture_load(const char* path, unsigned int format, unsigned int wrapping) {

  int width, height, channels;
  unsigned char *data = stbi_load(path, &width, &height, &channels, 0);
  if (data == NULL) {
    std::cout << "ERROR::TEXTURE: Unable to load " << path << "\n";
    exit(1);
  }

  unsigned int tid;
  glGenTextures(1, &tid);
  glBindTexture(GL_TEXTURE_2D, tid);
  {
    // Texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapping);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapping);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
  }
  glBindTexture(GL_TEXTURE_2D, 0);

  stbi_image_free(data);
  return {
    .id = tid
  };
}

void texture_bind(const texture_t &texture, int slot) {
  glActiveTexture(GL_TEXTURE0 + slot);
  glBindTexture(GL_TEXTURE_2D, texture.id);
}

#endif

} // namespace glib
