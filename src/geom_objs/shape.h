#pragma once

#include "pigment.h"
// #include "finish.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;

#define SHAPE_ID 0

class shape 
{
public:
	shape() : p(pigment())
	{
		reflectivity = 0;
	}
	shape(pigment p) : p(p)
	{
		reflectivity = 0;
	}
	// shape() : p(pigment()), f(finish()) {}
	// shape(pigment p) : p(p), f(finish()) {}
	// shape(pigment p, finish f) : p(p), f(f) {}
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
	pigment p;
	float reflectivity;
	// finish f;
};
