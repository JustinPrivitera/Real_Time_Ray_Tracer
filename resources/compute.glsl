#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
// #extension GL_ARB_compute_shader : enable

#define WIDTH 640
#define HEIGHT 480
#define RECURSION_DEPTH 50 // this isn't used yet
#define NUM_SHAPES 3

#define SPHERE_ID 1
#define PLANE_ID 5
// others

// one shader unit per pixel

layout(local_size_x = 1, local_size_y = 1) in;

layout(rgba32f, binding = 0) uniform image2D img_output;									//output image

layout (std430, binding = 0) volatile buffer shader_data
{ 
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
	// sphere: vec4 center, radius; vec4 nothing; vec4 color, shape_id
	// plane: vec4 normal, distance from origin; vec4 point in plane; vec4 color, shape_id

	// vec4 pixels[WIDTH][HEIGHT];
};

uniform int sizeofbuffer;

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
	{
		return -1;
	}
	else if (del == 0)
	{
		return -1 * dot_of_stuff; // hmmm i wonder why?
	}
	else // del > 0
	{
		float t1, t2, intermediate;
		intermediate = sqrt(del);
		t1 = -1 * dot_of_stuff + intermediate;
		t2 = -1 * dot_of_stuff - intermediate;
		if (t2 < 0)
		{
			if (t1 < 0)
			{
				return -1;
			}
			else
			{
				return t1;
			}
		}
		else
		{
			return t2;
		}
	}
	return -1;
}

float plane_eval_ray(vec3 pos, vec3 dir, int shape_index)
{
	vec3 normal = simple_shapes[shape_index][0].xyz;
	float denom = dot(normal, dir);
	if (denom < 0.001 && denom > -0.001)
		return -1;
	vec3 p0 = simple_shapes[shape_index][1].xyz;
	return dot(normal, p0 - pos) / denom;
}

float eval_ray(vec3 pos, vec3 dir, int shape_index)
{
	int shape_id = int(simple_shapes[shape_index][2].w);
	if (shape_id == SPHERE_ID)
	{
		return sphere_eval_ray(pos, dir, shape_index);
	}
	if (shape_id == PLANE_ID)
	{
		return plane_eval_ray(pos, dir, shape_index);
	}
	// other shapes?
	return -1;
}

bool shadow_ray(vec3 pos, int shape_index)
{
	double t;

	vec3 l = normalize(light_pos.xyz - pos);
	// new ray with p = pos and dir = l

	for (int i = 0; i < NUM_SHAPES; i ++)
	{
		t = eval_ray(pos, l, i);
		if (i == shape_index)
		{
			if (t > 0.0001)
				return false; // we are in shadow
		}
		else
			if (t > 0)
				return false; // we are in shadow
	}
	return true;
}

vec3 sphere_compute_normal(vec3 pos, int shape_index)
{
	return normalize(pos - simple_shapes[shape_index][0].xyz);
}

vec4 phong(vec3 dir) // phong diffuse lighting
{
	vec4 result_color;
	// basic ray casting
	float t = -1;
	float res_t;
	int ind = -1;

	for (int i = 0; i < NUM_SHAPES; i ++)
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
		// ok it'd be nice to have default code for no light sources
		// pixels[x][y] = simple_shapes[ind][2];

		// I guess we assume there is a light source for now
		vec3 curr_pos = camera_location.xyz + t * dir;
		bool lit = shadow_ray(curr_pos, ind);
		int shape_id = int(simple_shapes[ind][2].w);
		vec3 normal;
		if (shape_id == SPHERE_ID)
		{
			normal = sphere_compute_normal(curr_pos, ind);
		}
		else if (shape_id == PLANE_ID)
		{
			normal = simple_shapes[ind][0].xyz;
		}
		else
		{
			// uh oh
		}

		if (lit)
		{
			vec3 l = normalize(light_pos.xyz - curr_pos);
			result_color = vec4(simple_shapes[ind][2].xyz * clamp(dot(normal, l), 0, 1), 0);
		}
		else
		{
			result_color = vec4(0); // we are in shadow
		}
	}
	return result_color;
}

void main()
{
	// uint index = gl_GlobalInvocationID.x;

	uint x = gl_GlobalInvocationID.x;
	uint y = gl_GlobalInvocationID.y;
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

	// ray direction calculation
	float hp = float(x) / WIDTH;
	float vp = float(y) / HEIGHT;
	vec3 dir = normalize(llc_minus_campos.xyz + hp * horizontal.xyz + vp * vertical.xyz);

	vec4 result_color = phong(dir);

	float gamma = 1/2.2; // for gamma correction
	result_color = vec4(pow(result_color.r, gamma), pow(result_color.g, gamma), pow(result_color.b, gamma), 0);

	imageStore(img_output, pixel_coords, result_color);
	return;
}
