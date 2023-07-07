#version 330 core
out vec4 FragCol;

in vec2 uv;

// Output of geometry pass
uniform struct {
  sampler2D position;
  sampler2D normal;
  sampler2D color_spec;
} gbuffer;

#define LIGHT_CAPACITY 100
uniform struct light_point_t {
  vec3 position;
  vec3 color;
  vec2 attenuation;
} lights[LIGHT_CAPACITY];

uniform vec3 camera_pos;

vec3 light_point(vec3 N, vec3 V, vec3 P, vec3 position, vec2 attenuation, vec3 color) {
  
  vec3 L = normalize(position - P);
  float kD = max(dot(N, L), 0.0f);
  vec3 diffuse = color * kD * 

  vec3 R = reflect(-L, N);

  float kS = pow(max(dot(V, R), 0.0f), 64.0f);


}

#define AMBIENT 0.1f
void main() {
  
  vec3 P = texture(gbuffer.position,   uv);
  vec3 N = texture(gbuffer.normal,     uv);
  vec4 C = texture(gbuffer.color_spec, uv);

  vec3 color = C.rgb;
  vec3 spec  = C.a;

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
    float kA = 1.0 / (1.0 + light.attenuation.x * distance 
      + light.attenuation.y * distance * distance);

    result += color * kD * kA * light.color;
    result += spec  * kS * kA * light.color;
  }
  
  FragCol = vec4(result, 1.0f);
}
