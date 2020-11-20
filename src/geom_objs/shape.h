#pragma once

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;

#define SHAPE_ID 0

class shape 
{
public:
	shape() : color(vec3())
	{
		reflectivity = 1;
		emissive = false;
	}
	shape(vec3 color) : color(color)
	{
		reflectivity = 1;
		emissive = false;
	}
	virtual vec3 get_location() const {
		return vec3();
	}
	virtual string whoami() const {
		return "shape";
	}
	virtual int id() const {
		return SHAPE_ID;
	}

public:
	vec3 color;
	float reflectivity;
	bool emissive;
};
