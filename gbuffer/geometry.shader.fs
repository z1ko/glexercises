#version 330 core

layout (location = 0) out vec3 g_position;
layout (location = 1) out vec3 g_normal;
layout (location = 2) out vec4 g_color_spec;

in mat3 TBN;
in vec3 frag_pos;
in vec2 uv;

uniform struct {
  sampler2D diffuse;
  sampler2D specular;
  sampler2D normal;
} material;

void main() {
  
  // Get normal in world-space
  vec3 N = texture(material.normal, uv).xyz;
  N = normalize(N * 2.0 - 1.0);
  N = TBN * N;

  g_position = frag_pos;
  g_normal = N;
  g_color_spec = vec4(texture(material.diffuse, uv).rgb,
    texture(material.specular, uv).r);
}
