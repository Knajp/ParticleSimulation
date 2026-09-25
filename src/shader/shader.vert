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

layout(location = 0) out vec4 vertexColor;

void main()
{
  Particle p = particles[gl_VertexIndex];

  gl_Position = vec4(p.position, 0.0, 0.0);

  vertexColor = vec4(p.color, 1.0);
}
