#version 410 core
out vec4 color;
in vec2 vertex_tex;
uniform sampler2D tex;


void main()
{
vec4 tcol = texture(tex, vertex_tex);


/*
float aver = (tcol.r + tcol.g + tcol.b) / 3.0;
aver = pow(aver,5);
color.rgb = vec3(aver);
*/

color = tcol;
color.a=1;
}
