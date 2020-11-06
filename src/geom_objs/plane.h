#pragma once

#include "shape.h"
// #include "axis_aligned_box.h"

#define PLANE_ID 5

class plane : public shape {
public:
	plane() : normal(vec3()), dist_from_orig(0), shape{} {
		p0 = dist_from_orig * normal;
	}
	plane(vec3 normal, float dist_from_orig, pigment p) : 
	normal(normalize(normal)), dist_from_orig(dist_from_orig), shape{p} {
		p0 = dist_from_orig * normal;
	}
	// plane(vec3 normal, float dist_from_orig, pigment p, finish f) : 
	// normal(normal.normalize()), dist_from_orig(dist_from_orig), shape{p, f} {
	// 	p0 = dist_from_orig * normal;
	// }
	
	virtual string whoami() const override;
	// axis_aligned_box bounding_box() const;

	virtual int id() const override
	{
		return PLANE_ID;
	}

public:
	vec3 normal;
	float dist_from_orig;
	vec3 p0; // point in plane
	// pigment p;
};

// axis_aligned_box plane::bounding_box() const // if the plane is not flat then it is game over
// {
// 	return axis_aligned_box(-1 * vec3(9999, 9999, 9999) + p0, vec3(9999, -500, 9999) + p0, pigment());
// }

string plane::whoami() const {
	return "plane";
}
