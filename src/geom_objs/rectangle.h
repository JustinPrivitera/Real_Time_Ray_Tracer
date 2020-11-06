#pragma once

#include "shape.h"
#include "axis_aligned_box.h"
#include "../math/mat.h"

#define RECTANGLE_ID 3
#define AA_RECTANGLE_ID 4

class rectangle : public shape
{
public:
	rectangle(vec3 llv, vec3 right, vec3 up, pigment p) : 
		llv(llv), right(right), up(up), shape{p} {
			normal = right.cross(up).normalize(); // this might screw me up later
			// cout << normal << endl;
			// cout << endl;
		}
	rectangle() : llv(vec3()), normal(vec3()), right(vec3()), up(vec3()) {}

	virtual double eval_ray(ray* r) override;
	virtual vec3 compute_normal() const override;
	virtual string whoami() const override;

	virtual int id() const override
	{
		return RECTANGLE_ID;
	}

public:
	vec3 llv; // lower left vertex
	vec3 normal;
	vec3 right;
	vec3 up;
};

// returns the t that intersects with the plane
double rectangle::eval_ray(ray* r)
{
	vec3 p, v, n;
	v = r->dir;
	p = r->orig;
	n = normal;
	double denom = n.dot(v);
	if (denom < 0.00000001 && denom > -0.00000001)
		return -1;
	double t = n.dot(llv - p) / denom;
	if (t < 0)
		return -1;
	if (t == 0) // we are in the plane
		return -1;
		// cerr << "t should not equal zero" << endl;
	vec3 pt = r->eval(t);
	mat3 A = mat3(right, up, llv);
	vec3 vect = A.solve(pt);
	if (vect.x < 1 && vect.x > -0.001 && vect.y < 1 && vect.y > -0.001 && vect.z > 0)
		return t;
	else
		return -1;
}

vec3 rectangle::compute_normal() const {
	return normal;
}

string rectangle::whoami() const {
	return "rectangle";
}

/////////////////////////////////////////////////////////////

double max_2(double a, double b)
{
	if (a > b)
		return a;
	return b;
}

double min_2(double a, double b)
{
	if (a < b)
		return a;
	return b;
}

/////////////////////////////////////////////////////////////

class axis_aligned_rectangle : public shape
{
public:
	axis_aligned_rectangle(vec3 llv, vec3 right, vec3 up, pigment p) 
		: llv(llv), 
		  right(right), 
		  up(up),
		  normal(right.cross(up).normalize().cast_to_ints()),
		  xy_axis(false),
		  yz_axis(false),
		  zx_axis(false),
		  minl(0),
		  minw(0),
		  maxl(0),
		  maxw(0),
		  is_area_light(false),
		  shape{p}
	{
		do_assignments();
	}

	axis_aligned_rectangle(vec3 llv, vec3 right, vec3 up, pigment p, bool is_area_light) 
		: llv(llv), 
		  right(right), 
		  up(up),
		  normal(right.cross(up).normalize().cast_to_ints()),
		  xy_axis(false),
		  yz_axis(false),
		  zx_axis(false),
		  minl(0),
		  minw(0),
		  maxl(0),
		  maxw(0),
		  is_area_light(is_area_light),
		  shape{p}
	{
		do_assignments();
	}

	axis_aligned_rectangle() 
		: llv(vec3()),
		  right(vec3()), 
		  up(vec3()),
		  normal(vec3()),
		  xy_axis(false),
		  yz_axis(false),
		  zx_axis(false),
		  minl(0),
		  minw(0),
		  maxl(0),
		  maxw(0),
		  is_area_light(false),
		  shape{pigment()}
	{
		// do_assignments();
	}

	virtual double eval_ray(ray* r) override;
	virtual vec3 compute_normal() const override;
	virtual string whoami() const override;

	virtual int id() const override
	{
		return AA_RECTANGLE_ID;
	}
	
	void do_assignments()
	{
		vec3 top_corner = (llv + right + up);
		if (normal.x != 0)
		{
			yz_axis = true;
			minl = min_2(llv.y, top_corner.y);
			minw = min_2(llv.z, top_corner.z);
			maxl = max_2(llv.y, top_corner.y);
			maxw = max_2(llv.z, top_corner.z);
		}
		else if (normal.y != 0)
		{
			zx_axis = true;
			minl = min_2(llv.z, top_corner.z);
			minw = min_2(llv.x, top_corner.x);
			maxl = max_2(llv.z, top_corner.z);
			maxw = max_2(llv.x, top_corner.x);
		}
		else if (normal.z != 0)
		{
			xy_axis = true;
			minl = min_2(llv.x, top_corner.x);
			minw = min_2(llv.y, top_corner.y);
			maxl = max_2(llv.x, top_corner.x);
			maxw = max_2(llv.y, top_corner.y);
		}
		else
			cerr << "bad normal in axis_aligned_rectangle" << endl;
	}

public:
	vec3 llv; // lower left vertex
	vec3 normal;
	vec3 right;
	vec3 up;
	bool xy_axis, yz_axis, zx_axis;
	double minl, maxl, minw, maxw;
	bool is_area_light;
};

// returns the t that intersects with the plane
double axis_aligned_rectangle::eval_ray(ray* r)
{
	vec3 p, v, n;
	v = r->dir;
	p = r->orig;
	n = normal;
	double denom = n.dot(v);
	if (denom < 0.00000001 && denom > -0.00000001)
		return -1;
	double t = n.dot(llv - p) / denom;
	if (t < 0)
		return -1;
	if (t == 0) // camera position is in the plane
	{
		// cerr << "axis_aligned_rectangle::eval_ray - t should not equal zero" << endl;
		return -1;
	}
	vec3 pt = r->eval(t);
	if (xy_axis)
	{
		if (pt.x >= minl && pt.x <= maxl && pt.y >= minw && pt.y <= maxw)
			return t;
	}
	else if (yz_axis)
	{
		if (pt.y >= minl && pt.y <= maxl && pt.z >= minw && pt.z <= maxw)
			return t;
	}
	else if (zx_axis)
	{
		if (pt.z >= minl && pt.z <= maxl && pt.x >= minw && pt.x <= maxw)
			return t;
	}
	else
	{
		cerr << "axis error" << endl;
		return -1;
	}
	return -1; // we hit the plane but not inside the rectangle.
}

vec3 axis_aligned_rectangle::compute_normal() const {
	return normal;
}

string axis_aligned_rectangle::whoami() const {
	return "axis_aligned_rectangle";
}
