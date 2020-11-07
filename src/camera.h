#pragma once

#include <glm/gtc/type_ptr.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// using namespace std;
using namespace glm;

using namespace std;

class camera
{
public:
	camera(vec3 loc, vec3 u, vec3 r, vec3 look) : location(loc), up(u), right(r), look_at(look) {} 
	camera() : location(vec3()), up(vec3()), right(vec3()), look_at(vec3()) {} // default constructor

public:
	vec3 location;
	vec3 up;
	vec3 right;
	vec3 look_at;
};
