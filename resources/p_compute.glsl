#version 460 
#extension GL_ARB_shader_storage_buffer_object : require
// #extension GL_ARB_compute_shader : enable

// PHONG SHADER

#define WIDTH 440
#define HEIGHT 330
#define AA 4 // NO AA USED
#define RECURSION_DEPTH 20
#define NUM_SHAPES 10

#define NUM_FRAMES 8

#define SPHERE_ID 1
#define PLANE_ID 5
#define RECTANGLE_ID 3
// others

#define PHONG_SHADOW_MIN 0.06

// one shader unit per pixel

layout(rgba32f, binding = 0) uniform image2D img_output;

layout(local_size_x = 1, local_size_y = 1) in;

layout (std430, binding = 0) volatile buffer shader_data
{
	vec4 mode; // utility
	vec4 horizontal; // ray casting vector
	vec4 vertical; // ray casting vector
	vec4 llc_minus_campos; // ray casting vector
	vec4 camera_location; // ray casting vector
	vec4 light_pos; // for point lights only
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

float random(vec2 st) 
{
	return fract(
		sin(
			dot(
				st.xy,
				vec2(12.9898,78.233)
				)
			)
		* 43758.5453123);
}

float sphere_eval_ray(vec3 pos, vec3 dir, int shape_index)
{
	vec3 center = simple_shapes[shape_index][0].xyz;
	float radius = simple_shapes[shape_index][0].w;

	vec3 pos_minus_center = pos - center;

	float dot_of_stuff = dot(dir, pos_minus_center);

	float del = dot_of_stuff * dot_of_stuff - dot(pos_minus_center, pos_minus_center) + radius * radius;
	
	if (del < 0)
		return -1;
	else if (del == 0)
		return -1 * dot_of_stuff; // hmmm i wonder why?
	else // del > 0
	{
		float t1, t2, intermediate;
		intermediate = sqrt(del);
		t1 = -1 * dot_of_stuff + intermediate;
		t2 = -1 * dot_of_stuff - intermediate;
		if (t2 < 0)
		{
			if (t1 < 0)
				return -1;
			else
				return t1;
		}
		else
			return t2;
	}
	return -1;
}

float plane_eval_ray(vec3 pos, vec3 dir, int shape_index)
{
	vec3 normal = simple_shapes[shape_index][0].xyz;
	float denom = dot(normal, dir);
	if (denom < 0.001 && denom > -0.001)
		return -1;
	vec3 p0 = simple_shapes[shape_index][3].xyz;
	return dot(normal, p0 - pos) / denom;
}

float eval_ray(vec3 pos, vec3 dir, int shape_index)
{
	int shape_id = int(simple_shapes[shape_index][4].w);
	if (shape_id == SPHERE_ID)
	{
		return sphere_eval_ray(pos, dir, shape_index);
	}
	if (shape_id == PLANE_ID)
	{
		return plane_eval_ray(pos, dir, shape_index);
	}
	// if (shape_id == RECTANGLE_ID)
	// {
	// 	return rectangle_eval_ray(pos, dir, shape_index);
	// }
	// other shapes?
	return -1;
}

vec3 sphere_compute_normal(vec3 pos, int shape_index)
{
	return normalize(pos - simple_shapes[shape_index][0].xyz);
}

bool shadow_ray(vec3 pos)
{
	double t;

	vec3 light_vector = light_pos.xyz - pos;
	vec3 l = normalize(light_vector);
	float len = length(light_vector);
	// float t_max = light_vector.x / l.x;
	// new ray with p = pos and dir = l

	// fixes shadow issue
	vec3 new_pos = pos + 0.01 * l;

	for (int i = 0; i < int(mode.z); i ++)
	{
		t = eval_ray(new_pos, l, i);
		if (t > 0.0001)
			if (length(t * l) < len)
				return false; // we are in shadow
	}
	return true;
}

vec4 phong(vec3 dir) // phong diffuse lighting
{
	vec4 result_color;
	// basic ray casting
	float t = -1;
	float res_t;
	int ind = -1;

	// the following math gets the closest collision
	for (int i = 0; i < int(mode.z); i ++)
	{
		res_t = eval_ray(camera_location.xyz, dir, i);
		if (res_t > 0)
		{
			if (res_t < t || t < 0)
			{
				t = res_t; // t is always the smallest t value so far
				ind = i;
			}
		}
	}
	if (ind == -1) // ray intersected no geometry
	{
		result_color = background;
	}
	else // ray intersected something; we get it's color and write out
	{
		vec3 curr_pos = camera_location.xyz + t * dir;
		bool lit = shadow_ray(curr_pos);
		int shape_id = int(simple_shapes[ind][4].w);
		vec3 normal;
		if (shape_id == SPHERE_ID)
			normal = sphere_compute_normal(curr_pos, ind);
		else if (shape_id == PLANE_ID)
			normal = simple_shapes[ind][0].xyz;
		else {} // other shapes... this is not yet implemented

		if (lit)
		{
			vec3 l = normalize(light_pos.xyz - curr_pos);
			float spec = pow(clamp(dot(normalize(l - dir), normal), 0, 1), 500);

			result_color = vec4(
				simple_shapes[ind][4].xyz
					* clamp(dot(normal, l), PHONG_SHADOW_MIN, 1.0),
				0);

			result_color += vec4(spec * 1);
		}
		else
			result_color = vec4(simple_shapes[ind][4].xyz * vec3(PHONG_SHADOW_MIN), 0);
	}
	return result_color;
}

void main()
{
	ivec2 xy = ivec2(gl_GlobalInvocationID.xy);
	uint x = xy.x;
	uint y = xy.y;
	int frame = int(mode.y);

	vec4 result_color = vec4(0);

	// ray direction calculation
	float hp = float(x) / WIDTH;
	float vp = float(y) / HEIGHT;
	vec3 dir = normalize(llc_minus_campos.xyz + hp * horizontal.xyz + vp * vertical.xyz);

	result_color += phong(dir);

	// gamma correction
	float gamma = 1/2.2;
	result_color = vec4(pow(result_color.r, gamma), pow(result_color.g, gamma), pow(result_color.b, gamma), 0);

	// write image
	pixels[frame][x][y] = result_color;
	imageStore(img_output, ivec2(x,y), result_color);
}
