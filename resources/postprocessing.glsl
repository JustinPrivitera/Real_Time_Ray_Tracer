#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
// #extension GL_ARB_compute_shader : enable


#define WIDTH 640
#define HEIGHT 480
#define NUM_SHAPES 8

// one shader unit per pixel

layout(local_size_x = 1, local_size_y = 1) in;

layout(rgba32f, binding = 0) uniform image2D img_output;

layout (std430, binding = 0) volatile buffer shader_data
{
	vec4 mode; // utility
	vec4 w; // ray casting vector
	vec4 u; // ray casting vector
	vec4 v; // ray casting vector
	vec4 horizontal; // ray casting vector
	vec4 vertical; // ray casting vector
	vec4 llc_minus_campos; // ray casting vector
	vec4 camera_location; // ray casting vector
	vec4 background; // represents the background color
	vec4 light_pos; // for point lights only
	vec4 simple_shapes[NUM_SHAPES][3]; // shape buffer
	vec4 rand_buffer[2]; // stores random numbers needed for ray bounces
	// sphere: vec4 center, radius; vec4 nothing; vec4 color, shape_id
	// plane: vec4 normal, distance from origin; vec4 point in plane; vec4 color, shape_id

	// g buffer
	vec4 pixels[WIDTH][HEIGHT];
	vec4 normals_buffer[WIDTH][HEIGHT];
	vec4 depth_buffer[WIDTH][HEIGHT];
};

uniform int sizeofbuffer;

void main()
{
	uint x = gl_GlobalInvocationID.x;
	uint y = gl_GlobalInvocationID.y;

	vec4 color = pixels[x][y];
	vec4 normal = normals_buffer[x][y];
	vec4 depth = depth_buffer[x][y];

	if (normal.w > 0.99) // if it is not a background pixel
	{
		vec4 up_color, down_color, left_color, right_color;
		vec4 up_normal, down_normal, left_normal, right_normal;
		vec4 up_depth, down_depth, left_depth, right_depth;
		up_color = down_color = left_color = right_color = vec4(0);
		up_normal = down_normal = left_normal = right_normal = vec4(0);
		up_depth = down_depth = left_depth = right_depth = vec4(0);
		float u,d,l,r;
		u = d = l = r = 0;

		if (x + 1 < WIDTH)
		{
			right_color = pixels[x + 1][y];
			right_normal = normals_buffer[x + 1][y];
			right_depth = depth_buffer[x + 1][y];
			if (right_normal.w > 0.99) // it is not a background pixel
			{
				float n = clamp(dot(right_normal.xyz, normal.xyz), 0, 1);
				// float dd = 1 - clamp(abs(depth.x - right_depth.x), 0, 1);
				// float c = length(color.xyz - right_color.xyz);
				r = n;
			}
		}

		if (x - 1 >= 0)
		{
			left_color = pixels[x - 1][y];
			left_normal = normals_buffer[x - 1][y];
			left_depth = depth_buffer[x - 1][y];
			if (left_normal.w > 0.99) // it is not a background pixel
			{
				float n = clamp(dot(left_normal.xyz, normal.xyz), 0, 1);
				// float dd = 1 - clamp(abs(depth.x - left_depth.x), 0, 1);
				// float c = length(color.xyz - left_color.xyz);
				l = n;
			}
		}

		if (y + 1 < HEIGHT)
		{
			up_color = pixels[x][y + 1];
			up_normal = normals_buffer[x][y + 1];
			up_depth = depth_buffer[x][y + 1];
			if (up_normal.w > 0.99) // it is not a background pixel
			{
				float n = clamp(dot(up_normal.xyz, normal.xyz), 0, 1);
				// float dd = 1 - clamp(abs(depth.x - up_depth.x), 0, 1);
				// float c = length(color.xyz - up_color.xyz);
				u = n;
			}
		}

		if (y - 1 >= 0)
		{
			down_color = pixels[x][y - 1];
			down_normal = normals_buffer[x][y - 1];
			down_depth = depth_buffer[x][y - 1];
			if (down_normal.w > 0.99) // it is not a background pixel
			{
				float n = clamp(dot(down_normal.xyz, normal.xyz), 0, 1);
				// float dd = 1 - clamp(abs(depth.x - down_depth.x), 0, 1);
				// float c = length(color.xyz - down_color.xyz);
				d = n;
			}
		}

		color = 
			(color + u * up_color + d * down_color + l * left_color + r * right_color) 
			/ (1 + u + d + l + r);
	}

	// write image
	imageStore(img_output, ivec2(x,y), color);
	return;
}
