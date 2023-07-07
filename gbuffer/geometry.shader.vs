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
