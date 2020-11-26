#version 460 
#extension GL_ARB_shader_storage_buffer_object : require
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_arrays_of_arrays : enable

#define WIDTH 440
#define HEIGHT 330
#define AA 4
#define RECURSION_DEPTH 20
#define NUM_SHAPES 10

#define NUM_FRAMES 8

#define SPHERE_ID 1
#define PLANE_ID 5
#define RECTANGLE_ID 3
// others

// one shader unit per pixel

layout(local_size_x = 1, local_size_y = 1) in;

layout (std430, binding = 0) volatile buffer shader_data
{
	vec4 mode; // utility
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


	// g buffer
	vec4 pixels[WIDTH][HEIGHT];

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
void main()
{
	ivec2 xy = ivec2(gl_GlobalInvocationID.xy);
	uint x = xy.x;
	uint y = xy.y;
	int frame = int(mode.y);
	float depth_sum = 0;

	vec4 result_color = vec4(0);


	// // anti-aliasing
	// for (int aa = 1; aa < AA; aa ++)
	// {
	// 	int first = aa * 2;
	// 	int second = first + 1;
	// 	vec2 seed1 = vec2(rand_buffer[second].x, rand_buffer[first].y);
	// 	vec2 seed2 = vec2(rand_buffer[first].z, rand_buffer[second].w);
	// 	vec2 seed3 = vec2(rand_buffer[first].x, rand_buffer[second].y);
	// 	vec2 seed4 = vec2(rand_buffer[second].z, rand_buffer[first].w);

	// 	randy = normalize(vec2(
	// 			random(seed1 + xy * seed2 - xy + seed3), 
	// 			random(seed4 * xy - seed3 * xy * seed2))) / 6 - vec2(0.08333);
		
	// 	// ray direction calculation
	// 	float hp = (float(x) + randy.x) / WIDTH;
	// 	float vp = (float(y) + randy.y) / HEIGHT;
	// 	vec3 dir = normalize(llc_minus_campos.xyz + hp * horizontal.xyz + vp * vertical.xyz);

	// 	result_color += ambient_occlusion(dir, aa);
	// }

	// result_color /= AA;

	// // gamma correction

if(frame == 2)
	pixels[x][y] += vec4(0,float(y)/300.0f,0,1); 
	else
	pixels[x][y] += vec4(0,float(y)/400.0f,0,1); 
}
