#version 450

layout (location = 0) in vec4 fragmentColor;
layout (location = 0) out vec4 outColor;
void main()
{ 
  vec2 p = gl_PointCoord * 2.0 - 1.0;

  if (dot(p, p) > 1.0)
      discard;
  outColor = fragmentColor;
}
