#version 450 
#extension GL_ARB_shader_storage_buffer_object : require
// #extension GL_ARB_compute_shader : enable

#define WIDTH 440
#define HEIGHT 330
#define RECURSION_DEPTH 30
#define AA 4
#define NUM_SHAPES 10

#define NUM_FRAMES 8

#define SPHERE_ID 1
#define PLANE_ID 5
#define RECTANGLE_ID 3
// others

#define PHONG_SHADOW_MIN 0.06

// #define REFLECTION_DEGRADATION_CONSTANT 1

// one shader unit per pixel

layout(local_size_x = 1, local_size_y = 1) in;

layout (std430, binding = 0) volatile buffer shader_data
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

	// vec4 owie[5000000];
};

uniform int sizeofbuffer;

float det(mat2 matrix) 
{
    return matrix[0].x * matrix[1].y - matrix[0].y * matrix[1].x;
}

mat3 inverse(mat3 matrix) 
{
    vec3 row0 = matrix[0];
    vec3 row1 = matrix[1];
    vec3 row2 = matrix[2];

    vec3 minors0 = vec3(
        det(mat2(row1.y, row1.z, row2.y, row2.z)),
        det(mat2(row1.z, row1.x, row2.z, row2.x)),
        det(mat2(row1.x, row1.y, row2.x, row2.y))
    );
    vec3 minors1 = vec3(
        det(mat2(row2.y, row2.z, row0.y, row0.z)),
        det(mat2(row2.z, row2.x, row0.z, row0.x)),
        det(mat2(row2.x, row2.y, row0.x, row0.y))
    );
    vec3 minors2 = vec3(
        det(mat2(row0.y, row0.z, row1.y, row1.z)),
        det(mat2(row0.z, row0.x, row1.z, row1.x)),
        det(mat2(row0.x, row0.y, row1.x, row1.y))
    );

    mat3 adj = transpose(mat3(minors0, minors1, minors2));

    return (1.0 / dot(row0, minors0)) * adj;
}

vec3 solve(mat3 m, vec3 b)
{
	return inverse(m) * b;
}

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
	vec3 p0 = simple_shapes[shape_index][3].xyz;
	return dot(normal, p0 - pos) / denom;
}

float rectangle_eval_ray(vec3 pos, vec3 dir, int shape_index)
{
	vec3 normal = simple_shapes[shape_index][0].xyz;
	float denom = dot(normal, dir);
	if (denom < 0.001 && denom > -0.001)
		return -1;
	vec3 llc = simple_shapes[shape_index][3].xyz;
	float t = dot(normal, llc - pos) / denom;
	if (t <= 0)
		return -1;
	vec3 pt = pos + t * dir;
	vec3 right = simple_shapes[shape_index][2].xyz;
	vec3 up = simple_shapes[shape_index][1].xyz;
	mat3 A = mat3(right, up, llc);
	vec3 vect = solve(A, pt);
	if (vect.x < 1 && vect.x > -0.001 && vect.y < 1 && vect.y > -0.001 && vect.z > 0)
		return t;
	else
		return -1;
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
	if (shape_id == RECTANGLE_ID)
	{
		return rectangle_eval_ray(pos, dir, shape_index);
	}
	// other shapes?
	return -1;
}

vec3 sphere_compute_normal(vec3 pos, int shape_index)
{
	return normalize(pos - simple_shapes[shape_index][0].xyz);
}

vec3 get_pt_within_unit_sphere(int aa)
{
	int first = aa * 2;
	int second = first + 1;
	vec2 seed1 = vec2(rand_buffer[first].x, rand_buffer[first].y);
	vec2 seed2 = vec2(rand_buffer[first].z, rand_buffer[first].w);
	vec2 seed3 = vec2(rand_buffer[second].x, rand_buffer[second].y);
	vec2 seed4 = vec2(rand_buffer[second].z, rand_buffer[second].w);

	vec2 xy = gl_GlobalInvocationID.xy;

	return normalize(vec3(
				random(seed1 + xy * seed4) * 2 - 1,
				random(seed2 - xy * seed4) * 2 - 1,
				random(seed3 * xy + seed4) * 2 - 1));
}

// return a vec4 array: vec4 attenuation, vec4 pos + stop bit, vec4 dir + shape_ind
void ambient_occlusion_helper_first_time(inout vec4 array[3], int depth, int aa)
// vec4 ambient_occlusion(vec3 pos, vec3 dir, int depth, int last_ind)
{
	uint x = gl_GlobalInvocationID.x;
	uint y = gl_GlobalInvocationID.y;

	if (depth <= 0)
		array[0] = vec4(0);

	vec3 pos = array[1].xyz;
	vec3 dir = array[2].xyz;
	// int last_ind = int(array[2].w);

	float t = -1;
	float res_t;
	int ind = -1;

	// the following math just gets the closest collision
	for (int i = 0; i < int(mode.z); i ++)
	{
		res_t = eval_ray(pos, dir, i);
		if (res_t > 0.0001)
		{
			if (res_t < t || t < 0)
			{
				t = res_t;
				ind = i;
			}
		}
	}

	int flap = int(mode.y);

	if (ind != -1) // we must have hit something
	{
		vec4 attenuation = simple_shapes[ind][4];

		if (simple_shapes[ind][1].w > 0.99) // is this shape emmissive?
		{
			array[0] = attenuation;
			array[1].w = 1; // this means stop the recursion
			return;
		}

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

		normals_buffer[flap][x][y] = vec4(normal, 1);
		depth_buffer[flap][x][y] = vec4(t, 0, 0, 1);

		array[0] = attenuation;
		array[1] = vec4(curr_pos, 0);

		float reflect = simple_shapes[ind][3].w;
		if (reflect > 0.999) // no reflection
			array[2] = vec4(normalize(get_pt_within_unit_sphere(aa) + normal), 0);
		else // reflection
		{
			vec3 R = normalize((dir - 2 * (dot(dir, normal) * normal))); // reflection vector
			array[2] = vec4(normalize(R + reflect * get_pt_within_unit_sphere(aa)), ind);
		}
	}
	else // return background color
	{
		normals_buffer[flap][x][y] = vec4(0);
		depth_buffer[flap][x][y] = vec4(0);

		array[0] = background;
		array[1].w = 1; // this means stop the recursion
	}
}

// return a vec4 array: vec4 attenuation, vec4 pos + stop bit, vec4 dir + shape_ind
void ambient_occlusion_helper(inout vec4 array[3], int depth, int aa)
// vec4 ambient_occlusion(vec3 pos, vec3 dir, int depth, int last_ind)
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
	for (int i = 0; i < int(mode.z); i ++)
	{
		res_t = eval_ray(pos, dir, i);
		if (res_t > 0.0001)
		{
			if (res_t < t || t < 0)
			{
				t = res_t;
				ind = i;
			}
		}
	}

	if (ind != -1) // we must have hit something
	{
		vec4 attenuation = simple_shapes[ind][4];
		if (simple_shapes[ind][1].w > 0.9) // is this shape emmissive?
		{
			array[0] = attenuation;
			array[1].w = 1; // this means stop the recursion
			return;
		}

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

		array[0] = attenuation;
		array[1] = vec4(curr_pos, 0);

		float reflect = simple_shapes[ind][3].w;
		if (reflect > 0.999) // no reflection
			array[2] = vec4(normalize(get_pt_within_unit_sphere(aa) + normal), 0);
		else // reflection
		{
			vec3 R = normalize((dir - 2 * (dot(dir, normal) * normal))); // reflection vector
			array[2] = vec4(normalize(R + reflect * get_pt_within_unit_sphere(aa)), ind);
		}
	}
	else // return background color
	{
		array[0] = background;
		array[1].w = 1; // this means stop the recursion
	}
}

vec4 ambient_occlusion(vec3 dir, int aa)
{
	// vec4 result_color = vec4(1);
	vec4 ambient_occlusion_buffer[3];

	ambient_occlusion_buffer[1].xyz = camera_location.xyz;
	ambient_occlusion_buffer[1].w = 0; // stop bit is set to 0
	ambient_occlusion_buffer[2] = vec4(dir, 0);

	// do the first iteration outside
	ambient_occlusion_helper_first_time(ambient_occlusion_buffer, RECURSION_DEPTH, aa);
	vec4 result_color = ambient_occlusion_buffer[0];

	if (ambient_occlusion_buffer[1].w > 0.99)
		return result_color;

	int i = RECURSION_DEPTH - 1;
	while (i > 0)
	{
		ambient_occlusion_helper(ambient_occlusion_buffer, i, aa);
		result_color = result_color * ambient_occlusion_buffer[0];
		if (ambient_occlusion_buffer[1].w == 1) // the stop bit was set
			break;
		i -= 1;
	}
	return result_color;
}

void main()
{
	ivec2 xy = ivec2(gl_GlobalInvocationID.xy);

	vec4 result_color = vec4(0);

	vec2 randy;

	for (int aa = 0; aa < AA; aa ++)
	{
		int first = aa * 2;
		int second = first + 1;
		vec2 seed1 = vec2(rand_buffer[second].x, rand_buffer[first].y);
		vec2 seed2 = vec2(rand_buffer[first].z, rand_buffer[second].w);
		vec2 seed3 = vec2(rand_buffer[first].x, rand_buffer[second].y);
		vec2 seed4 = vec2(rand_buffer[second].z, rand_buffer[first].w);

		randy = normalize(vec2(
				random(seed1 + xy * seed2 - xy + seed3), 
				random(seed4 * xy - seed3 * xy * seed2))) / 4 - vec2(0.125);
		
		// ray direction calculation
		float hp = (float(xy.x) + randy.x) / WIDTH;
		float vp = (float(xy.y) + randy.y) / HEIGHT;
		vec3 dir = normalize(llc_minus_campos.xyz + hp * horizontal.xyz + vp * vertical.xyz);

		result_color += ambient_occlusion(dir, aa);
	}

	result_color /= AA;

	// gamma correction
	float gamma = 1/2.2;
	result_color = vec4(pow(result_color.r, gamma), pow(result_color.g, gamma), pow(result_color.b, gamma), 0);

	// write image
	pixels[int(mode.y)][xy.x][xy.y] = result_color;
	return;
}
