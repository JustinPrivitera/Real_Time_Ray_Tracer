#version 460 
#extension GL_ARB_shader_storage_buffer_object : require
// #extension GL_ARB_compute_shader : enable


#define WIDTH 440
#define HEIGHT 330
#define AA 4
#define NUM_SHAPES 10

#define NUM_FRAMES 8

// one shader unit per pixel

layout(local_size_x = 1, local_size_y = 1) in;

layout(rgba32f, binding = 0) uniform image2D img_output;

layout(std430, binding = 0) volatile buffer shader_data
{
	vec4 mode; // utility
	vec4 horizontal; // ray casting vector
	vec4 vertical; // ray casting vector
	vec4 llc_minus_campos; // ray casting vector
	vec4 camera_location; // ray casting vector
	vec4 background; // represents the background color
	vec4 simple_shapes[NUM_SHAPES][5]; // shape buffer
	// sphere:
		// vec4: vec3 center, float radius
		// vec4: vec3 nothing, bool emissive?
		// vec4: nothing
		// vec4: vec3 nothing, float reflectivity
		// vec4: vec3 color, int shape_id
	// plane:
		// vec4: vec3 normal, float distance from origin
		// vec4: vec3 nothing, bool emissive?
		// vec4: nothing
		// vec4: vec3 point in plane, float reflectivity
		// vec4: vec3 color, int shape_id
	// rectangle:
		// vec4: vec3 normal, float nothing
		// vec4: vec3 up, bool emmissive?
		// vec4: vec3 right, float nothing
		// vec4: vec3 lower left corner, float reflectivity
		// vec4: vec3 color, int shape_id

	vec4 rand_buffer[AA * 2]; // stores random numbers needed for ray bounces

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

	int new_frame = int(mode.y);

	vec4 color = pixels[new_frame][x][y];
	vec4 new_normal = normals_buffer[new_frame][x][y];

	if (new_normal.w > 0.99) // if it is not a background pixel
	{
		float new_depth = depth_buffer[new_frame][x][y].x;
		float new_bounces = depth_buffer[new_frame][x][y].y;

		// spatial
		vec4 left,right,up,down;
		float l,r,u,d;
		vec3 l_normal, r_normal, u_normal, d_normal;
		float l_depth, r_depth, u_depth, d_depth;
		float l_bounces, r_bounces, u_bounces, d_bounces;

		// vec4 upleft,upright,downleft,downright

		if (x < WIDTH)
		{
			right = pixels[new_frame][x + 1][y];
			r_normal = normals_buffer[new_frame][x + 1][y].xyz;
			if (normals_buffer[new_frame][x + 1][y].w < 0.001)
				r = 1;
			else
			{
				r_depth = depth_buffer[new_frame][x + 1][y].x;
				r_bounces = depth_buffer[new_frame][x + 1][y].y;

				float normal_dot = dot(new_normal.xyz, r_normal);
				float depth_diff = (1 - clamp(abs(new_depth - r_depth), 0, 1));
				float bounces_diff = (1 - clamp(abs(new_bounces - r_bounces) / 1.7, 0, 1));
				// float mag = length(right);
				r = normal_dot * depth_diff * bounces_diff + 0.2;
			}
		}
		else
		{
			r = 0;
		}

		if (x > 0)
		{
			left = pixels[new_frame][x - 1][y];
			l_normal = normals_buffer[new_frame][x - 1][y].xyz;
			if (normals_buffer[new_frame][x - 1][y].w < 0.001)
				l = 1;
			else
			{
				l_depth = depth_buffer[new_frame][x - 1][y].x;
				l_bounces = depth_buffer[new_frame][x - 1][y].y;

				float normal_dot = dot(new_normal.xyz, l_normal);
				float depth_diff = (1 - clamp(abs(new_depth - l_depth), 0, 1));
				float bounces_diff = (1 - clamp(abs(new_bounces - l_bounces) / 1.7, 0, 1));
				// float mag = length(left);
				l = normal_dot * depth_diff * bounces_diff + 0.2;
			}
		}
		else
		{
			l = 0;
		}

		if (y + 1 < HEIGHT)
		{
			up = pixels[new_frame][x][y + 1];
			u_normal = normals_buffer[new_frame][x][y + 1].xyz;
			if (normals_buffer[new_frame][x][y + 1].w < 0.001)
				u = 1;
			else
			{
				u_depth = depth_buffer[new_frame][x][y + 1].x;
				u_bounces = depth_buffer[new_frame][x][y + 1].y;

				float normal_dot = dot(new_normal.xyz, u_normal);
				float depth_diff = (1 - clamp(abs(new_depth - u_depth), 0, 1));
				float bounces_diff = (1 - clamp(abs(new_bounces - u_bounces) / 1.7, 0, 1));
				// float mag = length(up);
				u = normal_dot * depth_diff * bounces_diff + 0.2;
			}
		}
		else
		{
			u = 0;
		}

		if (y - 1 > 0)
		{
			down = pixels[new_frame][x][y - 1];
			d_normal = normals_buffer[new_frame][x][y - 1].xyz;
			if (normals_buffer[new_frame][x][y - 1].w < 0.001)
				d = 1;
			else
			{
				d_depth = depth_buffer[new_frame][x][y - 1].x;
				d_bounces = depth_buffer[new_frame][x][y - 1].y;

				float normal_dot = dot(new_normal.xyz, d_normal);
				float depth_diff = (1 - clamp(abs(new_depth - d_depth), 0, 1));
				float bounces_diff = (1 - clamp(abs(new_bounces - d_bounces) / 1.7, 0, 1));
				// float mag = length(down);
				d = normal_dot * depth_diff * bounces_diff + 0.2;
			}
		}
		else
		{
			d = 0;
		}

		color = (color + u * up + d * down + l * left + r * right) / (1 + u + d + l + r);

		// temporal AA

		vec4 color_sum = vec4(0);
		float denominator = 0.9;
		for (int i = 1; i < NUM_FRAMES; i ++)
		{
			int curr_frame = (new_frame + NUM_FRAMES - i) % NUM_FRAMES;
			vec3 curr_normal = normals_buffer[curr_frame][x][y].xyz;
			float curr_depth = depth_buffer[curr_frame][x][y].x;
			float curr_bounces = depth_buffer[curr_frame][x][y].y;

			float normal_dot = dot(new_normal.xyz, curr_normal);
			float depth_diff = (1 - clamp(abs(new_depth - curr_depth), 0, 1));
			float bounces_diff = (1 - clamp(abs(new_bounces - curr_bounces) / 1.7, 0, 1));

			float coeff = normal_dot * depth_diff * bounces_diff;

			if (coeff > 0.85)
			{
				color_sum += coeff * pixels[curr_frame][x][y];
				denominator += coeff;
			}
			else
				break;
		}

		color = (color * 0.9 + color_sum) / denominator;
	}

	pixels[new_frame][x][y] = color;

	// write image
	imageStore(img_output, ivec2(x,y), color);
	return;
}
