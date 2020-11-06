#pragma once

#include "shape.h"
#include "../math/mat.h"

#define TRIANGLE_ID 2

class triangle : public shape {
public:
	triangle() : a(vec3()), b(vec3()), c(vec3()), normal(vec3()), shape{pigment()} {}
	triangle(vec3 a, vec3 b, vec3 c, pigment p) : a(a), b(b), c(c), shape{p} {
		b_minus_a = b - a;
		c_minus_a = c - a;
		normal = b_minus_a.cross(c_minus_a).normalize();
	}
	triangle(vec3 a, vec3 b, vec3 c, vec3 normal, pigment p) : a(a), b(b), c(c), normal(normal), shape{p}
	{
		b_minus_a = b - a;
		c_minus_a = c - a;
		// normal = b_minus_a.cross(c_minus_a).normalize();
	}
	
	virtual double eval_ray(ray* r) override;
	virtual vec3 compute_normal() const override;
	virtual string whoami() const override;
	vec3 avg_pt() const;
	virtual axis_aligned_box bounding_box() const;

	virtual int id() const override
	{
		return TRIANGLE_ID;
	}


public:
	vec3 a,b,c;
	vec3 b_minus_a, c_minus_a;
	vec3 normal;
};

double max3(double u, double v, double w)
{
	if (u > v && u > w)
		return u;
	if (v > u && v > w)
		return v;
	if (w > u && w > v)
		return w;
	return u; // I guess they're all equal...
}

double max2(double a, double b)
{
	if (a > b)
		return a;
	return b;
}

double min2(double a, double b)
{
	if (a < b)
		return a;
	return b;
}

vec3 maxv(vec3 a, vec3 b)
{
	return vec3(max2(a.x,b.x),max2(a.y,b.y),max2(a.z,b.z));
}

vec3 minv(vec3 a, vec3 b)
{
	return vec3(min2(a.x,b.x),min2(a.y,b.y),min2(a.z,b.z));
}

double min3(double u, double v, double w)
{
	if (u < v && u < w)
		return u;
	if (v < u && v < w)
		return v;
	if (w < u && w < v)
		return w;
	return u; // I guess they're all equal...
}

axis_aligned_box triangle::bounding_box() const
{
	return axis_aligned_box(maxv(c, maxv(a,b)), minv(c, minv(a,b)), pigment());
}

vec3 triangle::avg_pt() const {
	return (a + b + c) / 3;
}

// returns the t that intersects with the triangle
double triangle::eval_ray(ray* r)
{
	// vec3 p = r->orig;
	// vec3 v = r->dir;
	// mat3 A = mat3(vec3() - b_minus_a, vec3() - c_minus_a, v);
	vec3 b = a - r->orig;

	vec3 col1 = vec3() - b_minus_a;
	vec3 col2 = vec3() - c_minus_a;
	vec3 col3 = r->dir;

	// double determinant = A.det();

	double aA = col1.x;
	double bA = col1.y;
	double cA = col1.z;
	double dA = col2.x;
	double eA = col2.y;
	double fA = col2.z;
	double gA = col3.x;
	double hA = col3.y;
	double iA = col3.z;

	double j = b.x;
	double k = b.y;
	double l = b.z;

	double ei_minus_hf = eA * iA - hA * fA;
	double gf_minus_di = gA * fA - dA * iA;
	double dh_minus_eg = dA * hA - eA * gA;
	double ak_minus_jb = aA * k - j * bA;
	double jc_minus_al = j * cA - aA * l;
	double bl_minus_kc = bA * l - k * cA;

	double determinant = aA * ei_minus_hf + bA * gf_minus_di + cA * dh_minus_eg;

	double beta = (j * ei_minus_hf + k * gf_minus_di + l * dh_minus_eg) / determinant;
	double gamma = (iA * ak_minus_jb + hA * jc_minus_al + gA * bl_minus_kc) / determinant;
	double t = -1 * (fA * ak_minus_jb + eA * jc_minus_al + dA * bl_minus_kc) / determinant;

	// we will solve Ax = b
	// vec3 X = A.solve(b);
	// double beta = X.x;
	// double gamma = X.y;
	// double t = X.z;

	if (beta >= 0 && gamma >= 0 && beta + gamma <= 1 && t > 0)
		return t;
	return -1;
}

vec3 triangle::compute_normal() const {
	return normal;
}

string triangle::whoami() const {
	return "triangle";
}
