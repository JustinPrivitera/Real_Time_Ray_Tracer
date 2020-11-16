#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
// #extension GL_ARB_compute_shader : enable


#define WIDTH 640
#define HEIGHT 480
#define RECURSION_DEPTH 30
#define NUM_SHAPES 8

// one shader unit per pixel

layout(local_size_x = 1, local_size_y = 1) in;

layout(rgba32f, binding = 0) uniform image2D img_output;

layout (std430, binding = 0) volatile buffer shader_data
{
	vec4 mode;
	// TODO add lighting mode and maybe some other selections
  	vec4 w;
	vec4 u;
	vec4 v;
	vec4 horizontal;
	vec4 vertical;
	vec4 llc_minus_campos;
	vec4 camera_location;
	vec4 background; // represents the background color
	vec4 light_pos; // for point lights only
	
	vec4 simple_shapes[NUM_SHAPES][3];
	// sphere: vec4 center, radius; vec4 nothing, type; vec4 color, shape_id
	// plane: vec4 normal, distance from origin; vec4 point in plane, type; vec4 color, shape_id

	 vec4 pixels[WIDTH][HEIGHT];
	// vec4 rand_buffer[WIDTH][HEIGHT];
	vec4 rand_buffer[2];
};

uniform int sizeofbuffer;

void main()
{
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

	// write image
	imageStore(img_output, pixel_coords, pixels[pixel_coords.x][pixel_coords.y]);
	return;
}
