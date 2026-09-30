#version 450
#extension GL_ARB_point_coord : enable

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

  vec2 centered = gl_PointCoord - vec2(0.5);

  if(length(centered) > 0.5) discard;

  gl_Position = vec4(p.position, 0.0, 1.0);
  gl_PointSize = 8.0;
  vertexColor = vec4(p.color, 1.0);
}
