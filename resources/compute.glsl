#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
// #extension GL_ARB_compute_shader : enable

#define WIDTH 640
#define HEIGHT 480
#define RECURSION_DEPTH 50

layout(local_size_x = 1, local_size_y = 1) in;
// we want to do a shader unit for each pixel


layout (std430, binding=0) volatile buffer shader_data
{ 
  	vec4 w;
	vec4 u;
	vec4 v;
	vec4 horizontal;
	vec4 vertical;
	vec4 llc_minus_campos;
	vec4 camera_location;
	vec4 background; // represents the background color
	// need a way to represent a scene
	vec4 pixels[WIDTH][HEIGHT];
};
// layout(location = 1) uniform uint even;

uniform int sizeofbuffer;

void main()
{
	// uint index = gl_GlobalInvocationID.x;

	uint x = gl_GlobalInvocationID.x;
	uint y = gl_GlobalInvocationID.y;

	pixels[x][y].xyz = vec3(1,0,0);

	// // ray direction calculation
	// float hp = float(x) / WIDTH;
	// float vp = float(y) / HEIGHT;
	// vec3 dir = llc_minus_campos + hp * horizontal + vp * vertical;

	return;
}
