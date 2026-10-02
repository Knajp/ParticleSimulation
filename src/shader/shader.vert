#version 450

struct Particle 
{
  vec2 position;
  vec3 color;
};

layout(std430, binding = 0) readonly buffer Particles
{
  Particle particles[];
};

layout(set = 0, binding = 1) uniform UniformBufferObject {
  mat4 proj;
} ubo; 

layout(location = 0) out vec4 vertexColor;

void main()
{
  Particle p = particles[gl_VertexIndex];



  gl_Position = ubo.proj * vec4(p.position, 0.0, 1.0);
  gl_PointSize = 8.0;
  vertexColor = vec4(p.color, 1.0);
}
