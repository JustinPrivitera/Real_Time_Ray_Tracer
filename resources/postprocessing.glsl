#version 460 
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_array_of_arrays : enable


#define WIDTH 440
#define HEIGHT 330
#define AA 4
#define NUM_SHAPES 10

#define NUM_FRAMES 8

// one shader unit per pixel

layout(local_size_x = 1, local_size_y = 1) in;

layout(rgba32f, binding = 0) uniform image2D img_output;

layout(std430, binding = 1) volatile buffer owiedata
{
	vec4 pixels[WIDTH][HEIGHT];
};

uniform int sizeofbuffer;

void main()
{
	uint x = gl_GlobalInvocationID.x;
	uint y = gl_GlobalInvocationID.y;

	vec4 color = pixels[x][y];

	// color += vec4(float(y) /500.0f, float(x)/500.0f,0,0);
	
	// write image
	imageStore(img_output, ivec2(x,y), color);
	return;
}
