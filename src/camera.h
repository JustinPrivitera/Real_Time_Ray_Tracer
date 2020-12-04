#pragma once

#include <glm/gtc/type_ptr.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// using namespace std;
using namespace glm;

using namespace std;

class camera
{
public:
	camera(vec3 loc, vec3 u, vec3 look) : location(loc), up(u), look_towards(look) {
		w = a = s = d = f = q = e = sp = ls = z = c = lighting = light_movement = v = 0;
	} 
	camera() : location(vec3()), up(vec3()), look_towards(vec3()) {
		w = a = s = d = f = q = e = sp = ls = z = c = lighting = light_movement = v = 0;
	} // default constructor

public:
	vec3 location;
	vec3 up;
	vec3 look_towards;
	// for keyboard input
	int w, a, s, d, f, q, e, sp, ls, z, c, lighting, light_movement, v;
};
