#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
// #extension GL_ARB_compute_shader : enable


#define WIDTH 320
#define HEIGHT 240
#define AA 10
#define NUM_SHAPES 3

#define NUM_FRAMES 16

// one shader unit per pixel

layout(local_size_x = 1, local_size_y = 1) in;

layout(rgba32f, binding = 0) uniform image2D img_output;

layout (std430, binding = 0) volatile buffer shader_data
{
	vec4 mode; // utility
	vec4 w[NUM_FRAMES]; // post processing
	vec4 horizontal; // ray casting vector
	vec4 vertical; // ray casting vector
	vec4 llc_minus_campos; // ray casting vector
	vec4 camera_location[NUM_FRAMES]; // ray casting vector
	vec4 background; // represents the background color
	// vec4 light_pos; // for point lights only
	vec4 simple_shapes[NUM_SHAPES][4]; // shape buffer
	// sphere:
		// vec4: vec3 center, float radius
		// vec4: vec3 nothing, bool emissive?
		// vec4: vec3 nothing, float reflectivity
		// vec4: vec3 color, int shape_id
	// plane:
		// vec4: vec3 normal, float distance from origin
		// vec4: vec3 nothing, bool emissive?
		// vec4: vec3 point in plane, float reflectivity
		// vec4: vec3 color, int shape_id

	vec4 rand_buffer[AA * 2]; // stores random numbers needed for ray bounces

	// g buffer
	vec4 pixels[NUM_FRAMES][WIDTH][HEIGHT];
	vec4 normals_buffer[NUM_FRAMES][WIDTH][HEIGHT];
	// vec4 depth_buffer[NUM_FRAMES][WIDTH][HEIGHT];
};

uniform int sizeofbuffer;

void main()
{
	uint x = gl_GlobalInvocationID.x;
	uint y = gl_GlobalInvocationID.y;

	int curr_frame = int(mode.y);

	vec4 color = pixels[curr_frame][x][y];
	vec4 curr_normal = normals_buffer[curr_frame][x][y];

	if (curr_normal.w > 0.99) // if it is not a background pixel
	{
		vec4 curr_campos = camera_location[curr_frame];
		vec4 curr_w = w[curr_frame];

		vec4 color_sum = vec4(0);
		float denominator = 1;
		for (int i = 1; i < NUM_FRAMES; i ++)
		{
			int oofus_frame = (curr_frame + NUM_FRAMES - i) % NUM_FRAMES;
			vec4 oofus_normal = normals_buffer[oofus_frame][x][y];
			vec4 oofus_campos = camera_location[oofus_frame];
			vec4 oofus_w = w[oofus_frame];

			if (
				dot(curr_normal, oofus_normal) > 0.7 && 
				length(curr_campos.xyz - oofus_campos.xyz) < 0.1 &&
				dot(curr_w, oofus_w) > 0.99)
			{
				color_sum += pixels[oofus_frame][x][y];
				denominator += 1;
			}
			else
				break;
		}

		color = (color + color_sum) / denominator;
	}

	// write image
	imageStore(img_output, ivec2(x,y), color);
	return;
}
