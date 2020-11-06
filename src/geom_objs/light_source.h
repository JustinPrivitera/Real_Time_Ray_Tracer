#pragma once

#include <glm/gtc/type_ptr.hpp>
// #include <glm/gtc/matrix_transform.hpp>
// using namespace std;
using namespace glm;

class area_light_source {
public:
	area_light_source() : right(vec3()), up(vec3()) {}
	area_light_source(vec3 right, vec3 up) : right(right), up(up) {}

public:
	vec3 right, up;
};

class light_source {
public:
	light_source() : position(vec3()), color(vec3()), area_light(area_light_source()), is_area_light(false) {}
	light_source(vec3 position, vec3 color) 
		: position(position), 
		color(color), 
		area_light(area_light_source()),
		is_area_light(false) {}
	light_source(vec3 position, vec3 color, area_light_source area_light, bool is_area_light) 
		: position(position), 
		color(color), 
		area_light(area_light),
		is_area_light(is_area_light) {}

public:
	vec3 position;
	vec3 color;
	area_light_source area_light;
	bool is_area_light;
};
