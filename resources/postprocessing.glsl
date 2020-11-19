#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
// #extension GL_ARB_compute_shader : enable


#define WIDTH 320
#define HEIGHT 240
#define AA 20
#define NUM_SHAPES 8

#define NUM_FRAMES 8

// one shader unit per pixel

layout(local_size_x = 1, local_size_y = 1) in;

layout(rgba32f, binding = 0) uniform image2D img_output;

layout (std430, binding = 0) volatile buffer shader_data
{
	vec4 mode; // utility
	vec4 w[NUM_FRAMES]; // ray casting vector
	// vec4 u; // ray casting vector
	// vec4 v; // ray casting vector
	vec4 horizontal; // ray casting vector
	vec4 vertical; // ray casting vector
	vec4 llc_minus_campos; // ray casting vector
	vec4 camera_location[NUM_FRAMES]; // ray casting vector
	vec4 background; // represents the background color
	vec4 light_pos; // for point lights only
	vec4 simple_shapes[NUM_SHAPES][3]; // shape buffer
	vec4 rand_buffer[AA * 2]; // stores random numbers needed for ray bounces
	// sphere: vec4 center, radius; vec4 nothing; vec4 color, shape_id
	// plane: vec4 normal, distance from origin; vec4 point in plane; vec4 color, shape_id

	// g buffer
	vec4 pixels[NUM_FRAMES][WIDTH][HEIGHT];
	vec4 normals_buffer[NUM_FRAMES][WIDTH][HEIGHT];
	vec4 depth_buffer[NUM_FRAMES][WIDTH][HEIGHT];
};

uniform int sizeofbuffer;

void main()
{
	uint x = gl_GlobalInvocationID.x;
	uint y = gl_GlobalInvocationID.y;

	int curr_frame = int(mode.y);

	// int frames[NUM_FRAMES];
	// vec4 colors[NUM_FRAMES];

	// frames[0] = int(mode.y); // current frame
	vec4 color = pixels[curr_frame][x][y];

	vec4 curr_normal = normals_buffer[curr_frame][x][y];

	// for (int i = 1; i < NUM_FRAMES; i ++)
	// {
	// 	frames[i] = (frames[0] + NUM_FRAMES - i) % NUM_FRAMES;
	// }

	// vec4 normals[NUM_FRAMES];
	// vec4 depths[NUM_FRAMES];

	
	// normals[0] = normals_buffer[frames[0]][x][y];
	// depths[0] = depth_buffer[frames[0]][x][y];

	if (curr_normal.w > 0.99) // if it is not a background pixel
	{
		vec4 curr_campos = camera_location[curr_frame];
		vec4 curr_w = w[curr_frame];

		// the oldest frame
		int last_frame = (curr_frame + 1) % NUM_FRAMES;
		vec4 last_normal = normals_buffer[last_frame][x][y];

		vec4 last_campos = camera_location[last_frame];
		vec4 last_w = w[last_frame];

		// if (
		// 	dot(curr_normal, last_normal) > 0.7 && 
		// 	length(curr_campos.xyz - last_campos.xyz) < 0.1 &&
		// 	dot(curr_w, last_w) > 0.99)
		{
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



		// for (int i = 1; i < NUM_FRAMES; i ++)
		// {
		// 	color[i] = pixels[frame[i]][x][y];
		// 	normals[i] = normals_buffer[frame[i]][x][y];
		// 	depths[i] = depth_buffer[frame[i]][x][y];


		// }


		// vec4 color2 = pixels[frame2][x][y];
		// vec4 normal2 = normals_buffer[frame2][x][y];
		// vec4 depth2 = depth_buffer[frame2][x][y];

		// float normal_dot2 = clamp(dot(normal1.xyz, normal2.xyz), 0, 1);
		// float depth_diff2 = 1 - clamp(abs(depth1.x - depth2.x), 0, 1);

		// float coeff2 = normal_dot2 * depth_diff2;

		// //////

		// vec4 color3 = pixels[frame3][x][y];
		// vec4 normal3 = normals_buffer[frame3][x][y];
		// vec4 depth3 = depth_buffer[frame3][x][y];

		// float normal_dot3 = clamp(dot(normal1.xyz, normal3.xyz), 0, 1);
		// float depth_diff3 = 1 - clamp(abs(depth1.x - depth3.x), 0, 1);

		// float coeff3 = normal_dot3 * depth_diff3;

		// //////

		// vec4 color4 = pixels[frame4][x][y];
		// vec4 normal4 = normals_buffer[frame4][x][y];
		// vec4 depth4 = depth_buffer[frame4][x][y];

		// float normal_dot4 = clamp(dot(normal1.xyz, normal4.xyz), 0, 1);
		// float depth_diff4 = 1 - clamp(abs(depth1.x - depth4.x), 0, 1);

		// float coeff4 = normal_dot4 * depth_diff4;

		// //////

		// color1 = (coeff2 * color2 + coeff3 * color3 + coeff4 * color4 + color1) 
		// 	/ (1 + coeff2 + coeff3 + coeff4);

		//////

		// vec4 up_color, down_color, left_color, right_color;
		// vec4 up_normal, down_normal, left_normal, right_normal;
		// vec4 up_depth, down_depth, left_depth, right_depth;
		// up_color = down_color = left_color = right_color = vec4(0);
		// up_normal = down_normal = left_normal = right_normal = vec4(0);
		// up_depth = down_depth = left_depth = right_depth = vec4(0);
		// float u,d,l,r;
		// u = d = l = r = 0;

		// if (x + 1 < WIDTH)
		// {
		// 	right_color = pixels[frame1][x + 1][y];
		// 	right_normal = normals_buffer[frame1][x + 1][y];
		// 	right_depth = depth_buffer[frame1][x + 1][y];
		// 	if (right_normal.w > 0.99) // it is not a background pixel
		// 	{
		// 		float n = clamp(dot(right_normal.xyz, normal1.xyz), 0, 1);
		// 		// float dd = 1 - clamp(abs(depth.x - right_depth.x), 0, 1);
		// 		// float c = length(color.xyz - right_color.xyz);
		// 		r = n;
		// 	}
		// }

		// if (x - 1 >= 0)
		// {
		// 	left_color = pixels[frame1][x - 1][y];
		// 	left_normal = normals_buffer[frame1][x - 1][y];
		// 	left_depth = depth_buffer[frame1][x - 1][y];
		// 	if (left_normal.w > 0.99) // it is not a background pixel
		// 	{
		// 		float n = clamp(dot(left_normal.xyz, normal1.xyz), 0, 1);
		// 		// float dd = 1 - clamp(abs(depth.x - left_depth.x), 0, 1);
		// 		// float c = length(color.xyz - left_color.xyz);
		// 		l = n;
		// 	}
		// }

		// if (y + 1 < HEIGHT)
		// {
		// 	up_color = pixels[frame1][x][y + 1];
		// 	up_normal = normals_buffer[frame1][x][y + 1];
		// 	up_depth = depth_buffer[frame1][x][y + 1];
		// 	if (up_normal.w > 0.99) // it is not a background pixel
		// 	{
		// 		float n = clamp(dot(up_normal.xyz, normal1.xyz), 0, 1);
		// 		// float dd = 1 - clamp(abs(depth.x - up_depth.x), 0, 1);
		// 		// float c = length(color.xyz - up_color.xyz);
		// 		u = n;
		// 	}
		// }

		// if (y - 1 >= 0)
		// {
		// 	down_color = pixels[frame1][x][y - 1];
		// 	down_normal = normals_buffer[frame1][x][y - 1];
		// 	down_depth = depth_buffer[frame1][x][y - 1];
		// 	if (down_normal.w > 0.99) // it is not a background pixel
		// 	{
		// 		float n = clamp(dot(down_normal.xyz, normal1.xyz), 0, 1);
		// 		// float dd = 1 - clamp(abs(depth.x - down_depth.x), 0, 1);
		// 		// float c = length(color.xyz - down_color.xyz);
		// 		d = n;
		// 	}
		// }

		// u = d = l = r = 1;

		// color1 = 
		// 	(color1 + u * up_color + d * down_color + l * left_color + r * right_color) 
		// 	/ (1 + u + d + l + r);
	}

	// write image
	imageStore(img_output, ivec2(x,y), color);
	return;
}
