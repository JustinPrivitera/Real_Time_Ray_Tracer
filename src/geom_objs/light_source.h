#pragma once

#include <glm/gtc/type_ptr.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// using namespace std;
using namespace glm;

class light_source {
public:
	light_source() : position(vec3()), color(vec3()) {}
	light_source(vec3 position, vec3 color) 
		: position(position), 
		color(color) {}

public:
	vec3 position;
	vec3 color;
};
