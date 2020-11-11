#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
// #extension GL_ARB_compute_shader : enable

#define WIDTH 640
#define HEIGHT 480
#define RECURSION_DEPTH 30
#define NUM_SHAPES 5

#define SPHERE_ID 1
#define PLANE_ID 5
// others

#define PHONG_SHADOW_MIN 0.06

// #define REFLECTION_DEGRADATION_CONSTANT 1

// one shader unit per pixel

layout(local_size_x = 1, local_size_y = 1) in;

layout(rgba32f, binding = 0) uniform image2D img_output;									//output image

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

	// vec4 pixels[WIDTH][HEIGHT];
	vec4 rand_buffer[WIDTH][HEIGHT];
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

	// the following math just gets the closest collision
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
		// result_color = simple_shapes[ind][2];

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
			// other shapes... this is not yet implemented
		}

		if (lit)
		{
			vec3 l = normalize(light_pos.xyz - curr_pos);
			result_color = vec4(
				simple_shapes[ind][2].xyz
					* clamp(dot(normal, l), PHONG_SHADOW_MIN, 1.0),
				0);
		}
		else
		{
			// result_color = vec4(0); // we are in shadow
			result_color = vec4(simple_shapes[ind][2].xyz * vec3(PHONG_SHADOW_MIN), 0);
		}
	}
	return result_color;
}

vec3 get_pt_within_unit_sphere()
{
	vec2 seed = vec2(mode.y, 1/ mode.y);

	uint x = gl_GlobalInvocationID.x;
	uint y = gl_GlobalInvocationID.y;

	vec2 seed1 = vec2(sin(x) + cos(y), sin(y) / cos(x));
	vec2 seed2 = vec2(cos(y) - 1/cos(y), cos(x) - sin(y));
	vec2 seed3 = vec2(sin(y), cos(x) * sin(x));

	// vec2 seed1 = vec2(pow(x,y), pow(y,x/y));
	// vec2 seed2 = vec2(seed1.x * pow(x,y - x), seed1.y * pow(y,x));
	// vec2 seed3 = vec2(seed2.y * pow(y,x + y), seed2.x + pow(y,x));

	// vec2 seed1 = vec2(x * seed.x, seed.x + seed.y);
	// vec2 seed2 = vec2(seed.y, y + seed.x * seed.y);
	// vec2 seed3 = vec2(seed.x / seed.y, x / y + seed.x * seed.y);

	return normalize(vec3(
				random(seed1) * 2 - 1,
				random(seed2) * 2 - 1,
				random(seed3) * 2 - 1));

	// return normalize(vec3(rand_buffer[x][y].x, rand_buffer[x][y].y, rand_buffer[x][y].z));
}

// return a vec4 array: vec4 attenuation, vec4 pos + stop bit, vec4 dir + shape_ind
void foggy_helper(inout vec4 array[3], int depth)
// vec4 foggy(vec3 pos, vec3 dir, int depth, int last_ind)
{
	if (depth <= 0)
		array[0] = vec4(0);

	vec3 pos = array[1].xyz;
	vec3 dir = array[2].xyz;
	int last_ind = int(array[2].w);

	float t = -1;
	float res_t;
	int ind = -1;

	// the following math just gets the closest collision
	for (int i = 0; i < NUM_SHAPES; i ++)
	{
		res_t = eval_ray(pos, dir, i);
		if (i == last_ind)
		{
			if (res_t > 0.0001)
			{
				if (res_t < t || t < 0)
				{
					t = res_t;
					ind = i;
				}
			}
		}
		else
		{
			if (res_t > 0)
			{
				if (res_t < t || t < 0)
				{
					t = res_t;
					ind = i;
				}
			}
		}
	}

	if (ind != -1) // we must have hit something
	{
		vec4 attenuation = simple_shapes[ind][2];
		vec3 curr_pos = camera_location.xyz + t * dir;
		int id = int(attenuation.w);
		vec3 normal;
		if (id == SPHERE_ID)
			normal = sphere_compute_normal(curr_pos, ind);
		else if (id == PLANE_ID)
			normal = simple_shapes[ind][0].xyz;
		else
		{
			// other shapes... this is not yet implemented
		}

		vec3 R = normalize((dir - 2 * (dot(dir, normal) * normal))); // reflection vector
		array[0] = attenuation;
		array[1] = vec4(curr_pos, 0);
		// array[2] = vec4(get_pt_within_unit_sphere() + normal, ind);
		array[2] = vec4(R, ind);
	}
	else // return background color
	{
		array[0] = background;
		array[1].w = 1; // this means stop the recursion
	}
}

vec4 foggy(vec3 dir)
{
	vec4 result_color = vec4(1);
	vec4 foggy_buffer[3];

	foggy_buffer[1].xyz = camera_location.xyz;
	foggy_buffer[1].w = 0; // stop bit is set to 0
	foggy_buffer[2] = vec4(dir, -1);

	int i = RECURSION_DEPTH;
	while (i > 0)
	{
		foggy_helper(foggy_buffer, i);
		result_color = result_color * foggy_buffer[0];
		if (foggy_buffer[1].w == 1) // we hit the background and the stop bit was set
			break;
		// result_color = result_color * foggy(camera_location.xyz, dir, i, -1);
		i -= 1;
	}
	return result_color;
}

// return a vec4 array: vec4 attenuation, vec4 pos + stop bit, vec4 dir + reflectivity
void hybrid_helper(inout vec4 array[3], int depth)
{
	if (depth <= 0)
		array[0] = vec4(0);

	vec3 pos = array[1].xyz;
	vec3 dir = array[2].xyz;
	// int last_ind = int(array[2].w);

	float t = -1;
	float res_t;
	int ind = -1;

	// the following math just gets the closest collision
	for (int i = 0; i < NUM_SHAPES; i ++)
	{
		res_t = eval_ray(pos, dir, i);
		if (res_t > 0.001)
		{
			if (res_t < t || t < 0)
			{
				t = res_t; // t is always the smallest t value so far
				ind = i;
			}
		}
	}
	if (ind == -1) // ray intersected no geometry so return background color
	{
		array[0] = background;
		array[1].w = 1; // this means stop the recursion
	}
	else // we hit something
	{
		vec4 attenuation = simple_shapes[ind][2];
		vec3 curr_pos = pos + t * dir;
		bool lit = shadow_ray(curr_pos, ind);
		int shape_id = int(attenuation.w);
		vec3 normal;
		if (shape_id == SPHERE_ID)
			normal = sphere_compute_normal(curr_pos, ind);
		else if (shape_id == PLANE_ID)
			normal = simple_shapes[ind][0].xyz;
		else {} // other shapes... this is not yet implemented

		// PHONG
		// add shadow feeler results to color at point
		if (lit)
		{
			vec3 l = normalize(light_pos.xyz - curr_pos);
			float spec = pow(clamp(dot(normalize(l - dir), normal), 0, 1), 500);

			attenuation = vec4(
				attenuation
					* clamp(
						dot(normal, l), 
						PHONG_SHADOW_MIN, 
						1.0));

			attenuation += vec4(spec * 1);
		}
		else // we are in shadow
		{
			attenuation = attenuation * vec4(PHONG_SHADOW_MIN);
		}

		// REFLECTION
		float reflectivity = simple_shapes[ind][1].w;
		if (reflectivity < 0.001) // not reflection
		{
			array[1].w = 1; // stop recursion
		}
		else // reflection
		{
			vec3 R = normalize((dir - 2 * (dot(dir, normal) * normal))); // reflection vector
			array[1].xyz = curr_pos;
			array[2] = vec4(R, reflectivity);
		}

		array[0] = attenuation;
	}
}

// every ray bounce, find phong lighting at point
// then if it is reflective, add phong lighting to a new bounce times the current reflection constant
// otherwise, return phong lighting

vec4 hybrid(vec3 dir)
{
	// vec4 result_color = vec4(1);
	vec4 lighting_buffer[3];

	lighting_buffer[1].xyz = camera_location.xyz;
	lighting_buffer[1].w = 0; // stop bit is set to 0
	lighting_buffer[2] = vec4(dir, 0);

	// do the first iteration outside
	hybrid_helper(lighting_buffer, RECURSION_DEPTH);
	float c = lighting_buffer[2].w;
	vec4 result_color = lighting_buffer[0];

	// if reflective, make result color = (res_col + c * hybrid) / (1 + c)
	int i = RECURSION_DEPTH - 1;
	while (i > 0)
	{
		hybrid_helper(lighting_buffer, i);
		result_color = (result_color + c * lighting_buffer[0]) / (1 + c);
		c = c * lighting_buffer[2].w;
		if (lighting_buffer[1].w == 1) // the stop bit was set
			break;
		i -= 1;
	}
	return result_color;
}

void main()
{
	ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

	vec4 result_color;
	
	// ray direction calculation
	float hp = float(pixel_coords.x) / WIDTH;
	float vp = float(pixel_coords.y) / HEIGHT;
	vec3 dir = normalize(llc_minus_campos.xyz + hp * horizontal.xyz + vp * vertical.xyz);

	// get color based on chosen lighting algorithm
	
	if (mode.x == 0)
		result_color = phong(dir);
	// else if (mode.x == 1)
	// 	result_color = foggy(dir);
	else
		result_color = hybrid(dir);

	// gamma correction
	float gamma = 1/2.2;
	// gamma = 1000 * mode.y + gamma;

	result_color = vec4(pow(result_color.r, gamma), pow(result_color.g, gamma), pow(result_color.b, gamma), 0);


	// write image
	imageStore(img_output, pixel_coords, result_color);
	return;
}
