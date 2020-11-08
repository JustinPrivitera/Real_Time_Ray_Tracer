#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
// #extension GL_ARB_compute_shader : enable

#define WIDTH 640
#define HEIGHT 480
#define RECURSION_DEPTH 50
#define NUM_SHAPES 3

#define SPHERE_ID 1
#define PLANE_ID 5
// others

// one shader unit per pixel

layout(local_size_x = 1, local_size_y = 1) in;

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
	
	// vec4 light_pos;
	vec4 simple_shapes[NUM_SHAPES][3];
	// sphere: vec4 center, radius; vec4 nothing; vec4 color, shape_id
	// plane: vec4 normal, distance from origin; vec4 point in plane; vec4 color, shape_id

	vec4 pixels[WIDTH][HEIGHT];
};

uniform int sizeofbuffer;

float sphere_eval_ray(vec3 dir, int shape_index)
{

	vec3 center = simple_shapes[shape_index][0].xyz;
	float radius = simple_shapes[shape_index][0].w;

	vec3 campos = camera_location.xyz;

	vec3 campos_minus_center = campos - center;

	float dot_of_stuff = dot(dir, campos_minus_center);

	float del = dot_of_stuff * dot_of_stuff - dot(campos_minus_center, campos_minus_center) + radius * radius;
	

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

float plane_eval_ray(vec3 dir, int shape_index)
{
	vec3 normal = simple_shapes[shape_index][0].xyz;
	float denom = dot(normal, dir);
	if (denom < 0.001 && denom > -0.001)
		return -1;
	vec3 p0 = simple_shapes[shape_index][1].xyz;
	return dot(normal, p0 - camera_location.xyz) / denom;
}

float eval_ray(vec3 dir, int shape_index)
{
	int shape_id = int(simple_shapes[shape_index][2].w);
	if (shape_id == SPHERE_ID)
	{
		return sphere_eval_ray(dir, shape_index);
	}
	if (shape_id == PLANE_ID)
	{
		return plane_eval_ray(dir, shape_index);
	}
	// other shapes?
	return -1;
}

void main()
{
	// uint index = gl_GlobalInvocationID.x;

	uint x = gl_GlobalInvocationID.x;
	uint y = gl_GlobalInvocationID.y;

	// ray direction calculation
	float hp = float(x) / WIDTH;
	float vp = float(y) / HEIGHT;
	vec3 dir = normalize(llc_minus_campos.xyz + hp * horizontal.xyz + vp * vertical.xyz);

	// basic ray casting
	float t = -1;
	float res_t;
	int ind = -1;

	for (int i = 0; i < NUM_SHAPES; i ++)
	{
		res_t = eval_ray(dir, i);
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
		pixels[x][y] = background;
	}
	else // ray intersected something; we get it's color and write out
	{
		pixels[x][y] = simple_shapes[ind][2];
		// pixels[x][y] = vec4(0);
	}

	return;
}
