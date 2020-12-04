#pragma once

class scene
{
public:
	scene(vector<shape*> shapes, vector<light_source> lights) : 
		shapes(shapes), lights(lights) {}
	scene() : shapes(vector<shape*>()), lights(vector<light_source>()) {}

public:
	vector<shape*> shapes;
	vector<light_source> lights;
};

scene init_scene1()
{
	// sphere
	vec3 center = vec3(0,-0.5,0);
	float radius = 2;
	vec3 color = vec3(0.8,0.2,0.5);
	sphere* mysphere = new sphere(center,radius,color);
	mysphere->reflectivity = 0.5;

	// sphere
	center = vec3(4,-0.5,-2);
	radius = 3.5;
	color = vec3(0.8,0.8,0.1);
	sphere* mysphere2 = new sphere(center,radius,color);
	mysphere2->reflectivity = 0.9;

	// sphere
	center = vec3(-4.5,4,-15);
	radius = 4;
	color = vec3(0.2,0.8,0.1);
	sphere* mysphere3 = new sphere(center,radius,color);
	mysphere3->reflectivity = 0.2;

	// sphere
	center = vec3(-8,-1,2);
	radius = 1.5;
	color = vec3(1,1,1);
	sphere* mysphere4 = new sphere(center,radius,color);
	mysphere4->reflectivity = 0;

	// plane
	vec3 normal = vec3(0, 1, 0);
	float distance_from_origin = -4;
	color = vec3(0.3,0.0,0.5);
	plane* myplane = new plane(normal, distance_from_origin, color);
	
	// shapes vector
	vector<shape*> myshapes = vector<shape*>();
	myshapes.push_back(mysphere);
	myshapes.push_back(mysphere2);
	myshapes.push_back(mysphere3);
	myshapes.push_back(mysphere4);
	myshapes.push_back(myplane);

	// light sources
	vector<light_source> lights = vector<light_source>();

	// make scene
	scene scene1 = scene(myshapes,lights);
	return scene1;
}

scene init_scene5()
{
	vector<light_source> lights = vector<light_source>();
	
	vector<shape*> myshapes5 = vector<shape*>();

	// sphere
	vec3 center = vec3(0,18,0);
	float radius = 10;
	vec3 color = vec3(1.5,1.5,1.5);
	sphere* s5sphere1 = new sphere(center,radius,color);
	s5sphere1->emissive = true;

	// // rectangle
	// vec3 llc = vec3(4,6,4);
	// vec3 up = vec3(-8,0,0);
	// vec3 right = vec3(0,0,-8);
	// color = vec3(1.5,1.5,1.5);
	// rectangle* s5rect1 = new rectangle(llc, right, up, color);
	// s5rect1->emissive = true;

	// sphere
	center = vec3(0,0,0);
	radius = 2;
	color = vec3(0.2,0.6,0.8);
	sphere* s5sphere2 = new sphere(center,radius,color);
	s5sphere2->reflectivity = 0.4;

	// sphere
	center = vec3(0,-35,0);
	radius = 33;
	color = vec3(0.8,0.6,0.2);
	sphere* s5sphere3 = new sphere(center,radius,color);

	myshapes5.push_back(s5sphere1);
	myshapes5.push_back(s5sphere2);
	myshapes5.push_back(s5sphere3);

	// make scene
	scene scene5 = scene(myshapes5,lights);

	return scene5;
}

scene init_scene6()
{
	vector<light_source> lights = vector<light_source>();
	vector<shape*> myshapes6 = vector<shape*>();

	// sphere
	vec3 center = vec3(0,12,0);
	float radius = 6;
	vec3 color = vec3(4,4,4);
	sphere* s6sphere1 = new sphere(center,radius,color);
	s6sphere1->emissive = true;

	// sphere
	center = vec3(-8,0,0);
	radius = 2;
	color = vec3(8, 8, 16);
	sphere* s6sphere2 = new sphere(center,radius,color);
	s6sphere2->emissive = true;

	// sphere
	center = vec3(0,0,0);
	radius = 2;
	color = vec3(0.2,0.6,0.8);
	sphere* s6sphere3 = new sphere(center,radius,color);
	s6sphere3->reflectivity = 0.4;

	// sphere
	center = vec3(0,-35,0);
	radius = 33;
	color = vec3(0.8,0.6,0.2);
	sphere* s6sphere4 = new sphere(center,radius,color);

	// sphere
	center = vec3(2,1,3);
	radius = 0.5;
	color = vec3(1,1,1);
	sphere* s6sphere5 = new sphere(center,radius,color);
	s6sphere5->reflectivity = 0.0;

	// sphere
	center = vec3(4.5, 0.2, 5);
	radius = 2.25;
	color = vec3(1,1,1);
	sphere* s6sphere6 = new sphere(center,radius,color);
	s6sphere6->reflectivity = 0.0;

	myshapes6.push_back(s6sphere1);
	myshapes6.push_back(s6sphere2);
	myshapes6.push_back(s6sphere3);
	myshapes6.push_back(s6sphere4);
	myshapes6.push_back(s6sphere5);
	myshapes6.push_back(s6sphere6);

	// make scene
	scene scene6 = scene(myshapes6,lights);
	return scene6;
}
