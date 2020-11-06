#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
// #extension GL_ARB_compute_shader : enable

#define WIDTH 10
#define HEIGHT 10
#define RECURSION_DEPTH 50

layout(local_size_x = WIDTH, local_size_y = HEIGHT) in;	
// we want to do a shader unit for each pixel


layout (std430, binding=0) volatile buffer shader_data
{ 
  	vec3 w;
	vec3 u;
	vec3 v;
	vec3 horizontal;
	vec3 vertical;
	vec3 llc;
	// need a way to represent a scene
	vec3 pixels[WIDTH][HEIGHT];
};
// layout(location = 1) uniform uint even;

uniform int sizeofbuffer;

void main()
{
	uint index = gl_GlobalInvocationID.x;

	return;
}
