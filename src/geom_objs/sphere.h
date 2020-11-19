#pragma once

#include "shape.h"
// #include "axis_aligned_box.h"

#define SPHERE_ID 1

class sphere : public shape {
public:
	sphere() : location(vec3()), radius(0), shape{} {}
	sphere(vec3 loc, float rad, pigment p) : location(loc), radius(rad), shape{p} {}
	// sphere(vec3 loc, float rad, pigment p, finish f) : location(loc), radius(rad), shape{p, f} {}
	
	virtual vec3 get_location() const override;
	virtual string whoami() const override;
	// virtual axis_aligned_box bounding_box() const;

	virtual int id() const override
	{
		return SPHERE_ID;
	}

public:
	vec3 location; // center
	float radius;
	// vec3 last_normal;
	// pigment p;
};

vec3 sphere::get_location() const {
	return location;
}

string sphere::whoami() const {
	return "sphere";
}
