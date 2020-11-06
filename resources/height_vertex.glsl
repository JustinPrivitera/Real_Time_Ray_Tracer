#version 330 core
layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertTex;

uniform mat4 P;
uniform mat4 V;
uniform mat4 M;
out vec3 vertex_pos;
out vec2 vertex_tex;
uniform sampler2D tex;

uniform vec3 camoff;

void main()
{

	vec4 tpos =  vec4(vertPos, 1.0);
	tpos =  M * tpos;
	gl_Position = P * V * tpos;
	vertex_tex = vertTex;
}
