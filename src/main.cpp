#include <fstream>
#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <glad/glad.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "GLSL.h"
#include "Program.h"
#include <time.h>
#include <random>
#include <math.h>
#include "MatrixStack.h"

#include "geomObj.h"
#include "camera.h"

#include "WindowManager.h"
#include "Shape.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;
// shared_ptr<Shape> shape;

// resolution
#define WIDTH 440
#define HEIGHT 330
#define AA 4

// number of scene objects
#define NUM_SHAPES 10

#define NUM_FRAMES 8

// aspect ratio constants
#define ASPECT_RATIO 1.333333 // horizontal
#define FULLSCREEN_ASPECT_RATIO 1.777777 // horizontal
#define VERT_ASPECT_RATIO 1.0

class ssbo_data
{
public:
	vec4 mode; // utility
	vec4 horizontal; // ray casting vector
	vec4 vertical; // ray casting vector
	vec4 llc_minus_campos; // ray casting vector
	vec4 camera_location; // ray casting vector
	vec4 background; // represents the background color
	vec4 simple_shapes[NUM_SHAPES][5]; // shape buffer
	// sphere:
		// vec4: vec3 center, float radius
		// vec4: vec3 nothing, bool emissive?
		// vec4: nothing
		// vec4: vec3 nothing, float reflectivity
		// vec4: vec3 color, int shape_id
	// plane:
		// vec4: vec3 normal, float distance from origin
		// vec4: vec3 nothing, bool emissive?
		// vec4: nothing
		// vec4: vec3 point in plane, float reflectivity
		// vec4: vec3 color, int shape_id
	// rectangle:
		// vec4: vec3 normal, float nothing
		// vec4: vec3 up, bool emmissive?
		// vec4: vec3 right, float nothing
		// vec4: vec3 lower left corner, float reflectivity
		// vec4: vec3 color, int shape_id

	vec4 rand_buffer[AA * 2]; // stores random numbers needed for ray bounces

	// g buffer
	vec4 pixels[NUM_FRAMES][WIDTH][HEIGHT];
	vec4 normals_buffer[NUM_FRAMES][WIDTH][HEIGHT];
	vec4 depth_buffer[NUM_FRAMES][WIDTH][HEIGHT];
};


double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}

class fake_camera
{
public:
	glm::vec3 pos, rot;
	int w, a, s, d, f, q, e, sp, ls, z, c, p, one, two, three;
	fake_camera()
	{
		w = a = s = d = f = q = e = sp = ls = z = c = p = one = two = three = 0;
		pos = rot = glm::vec3(0, 0, 0);
	}
	glm::mat4 process(double ftime)
	{
		float speed = 0;
		if (w == 1)
		{
			speed = 10*ftime;
		}
		else if (s == 1)
		{
			speed = -10*ftime;
		}
		float yangle=0;
		if (a == 1)
			yangle = -3*ftime;
		else if(d==1)
			yangle = 3*ftime;
		rot.y += yangle;
		glm::mat4 R = glm::rotate(glm::mat4(1), rot.y, glm::vec3(0, 1, 0));
		glm::vec4 dir = glm::vec4(0, 0, speed,1);
		dir = dir*R;
		pos += glm::vec3(dir.x, dir.y, dir.z);
		glm::mat4 T = glm::translate(glm::mat4(1), pos);
		return R*T;
	}
};

// #define initpos vec3(0,0,-20);
fake_camera mycam;
ofstream outFile;

float randf()
{
	return (float)(rand() / (float)RAND_MAX);
}

inline std::ostream& operator<<(std::ostream &out, const vec3 &v)
{ 
	return out << v.x << ", " << v.y << ", " << v.z;
}

inline std::ostream& operator<<(std::ostream &out, const vec4 &v)
{ 
	return out << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
}

inline vec3 operator*(const vec3 &v, int Sc) {
	return vec3(v.x * Sc, v.y * Sc, v.z * Sc);
}

inline vec3 operator*(const vec3 &v, double Sc) {
	return vec3(v.x * Sc, v.y * Sc, v.z * Sc);
}

inline vec3 operator*(double Sc, const vec3 &v) {
	return vec3(v.x * Sc, v.y * Sc, v.z * Sc);
}

inline vec3 operator*(int Sc, const vec3 &v) {
	return vec3(v.x * Sc, v.y * Sc, v.z * Sc);
}

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

class Application : public EventCallbacks
{

public:

	float aspect_ratio = ASPECT_RATIO;
	int true_num_scene_objects = 5; // NUM_SHAPES;
	// int light_movement = 0;

	scene scene1 = init_scene1();
	scene scene5 = init_scene5();
	scene scene6 = init_scene6();

	scene myscene = scene1;

	// copies of the SSBO data since these values will change each frame
	vec3 w;
	vec3 u;
	vec3 v;
	vec3 horizontal;
	vec3 vertical;
	vec3 llc_minus_campos;
	// end

	// build ray trace camera
	vec3 location = vec3(0,0,14);
	vec3 up = vec3(0,1,0);
	vec3 look_towards = vec3(0,0,1);
	camera rt_camera = camera(location, up, look_towards);
	// end

	WindowManager * windowManager = nullptr;

	ssbo_data ssbo_CPUMEM;
	GLuint ssbo_GPU_id;
	GLuint computeProgram, postProcessingProgram;

	// Our shader program
	std::shared_ptr<Program> prog, heightshader;
	// Contains vertex information for OpenGL
	GLuint VertexArrayID, VertexArrayIDScreen;

	// Data necessary to give our box to OpenGL
	GLuint VertexBufferID, VertexBufferTexScreen, VertexBufferIDScreen,VertexNormDBox, VertexTexBox, IndexBufferIDBox, InstanceBuffer;

	//framebufferstuff
	GLuint fb, depth_fb, FBOtex;
	//texture data
	GLuint Texture, Texture2;
	GLuint CS_tex_A, CS_tex_B;

	int tex_w, tex_h;

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

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		
		if (key == GLFW_KEY_W && action == GLFW_PRESS)
		{
			mycam.w = 1;
		}
		if (key == GLFW_KEY_W && action == GLFW_RELEASE)
		{
			mycam.w = 0;
		}
		if (key == GLFW_KEY_S && action == GLFW_PRESS)
		{
			mycam.s = 1;
		}
		if (key == GLFW_KEY_S && action == GLFW_RELEASE)
		{
			mycam.s = 0;
		}
		if (key == GLFW_KEY_A && action == GLFW_PRESS)
		{
			mycam.a = 1;
		}
		if (key == GLFW_KEY_A && action == GLFW_RELEASE)
		{
			mycam.a = 0;
		}
		if (key == GLFW_KEY_D && action == GLFW_PRESS)
		{
			mycam.d = 1;
		}
		if (key == GLFW_KEY_D && action == GLFW_RELEASE)
		{
			mycam.d = 0;
		}
		// move up and down
		if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS)
		{
			mycam.ls = 1;
		}
		if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE)
		{
			mycam.ls = 0;
		}
		if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
		{
			mycam.sp = 1;
		}
		if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
		{
			mycam.sp = 0;
		}

		// camera rotation
		if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		{
			mycam.q = 1;
		}
		if (key == GLFW_KEY_Q && action == GLFW_RELEASE)
		{
			mycam.q = 0;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS)
		{
			mycam.e = 1;
		}
		if (key == GLFW_KEY_E && action == GLFW_RELEASE)
		{
			mycam.e = 0;
		}

		// camera rotation
		if (key == GLFW_KEY_Z && action == GLFW_PRESS)
		{
			mycam.z = 1;
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE)
		{
			mycam.z = 0;
		}
		if (key == GLFW_KEY_C && action == GLFW_PRESS)
		{
			mycam.c = 1;
		}
		if (key == GLFW_KEY_C && action == GLFW_RELEASE)
		{
			mycam.c = 0;
		}

		// toggle scene
		if (key == GLFW_KEY_1 && action == GLFW_PRESS)
		{
			mycam.one = 1;
			mycam.two = 0;
			mycam.three = 0;
			// ssbo_CPUMEM.background = vec4(1);
			ssbo_CPUMEM.background = vec4(13/255.0, 153/255.0, 219/255.0, 0);
			true_num_scene_objects = 5;
			myscene = scene1;
		}

		// toggle scene
		if (key == GLFW_KEY_2 && action == GLFW_PRESS)
		{
			mycam.one = 0;
			mycam.two = 1;
			mycam.three = 0;
			ssbo_CPUMEM.background = vec4(0);
			true_num_scene_objects = 3;
			myscene = scene5;
		}

		// toggle scene
		if (key == GLFW_KEY_3 && action == GLFW_PRESS)
		{
			mycam.one = 0;
			mycam.two = 0;
			mycam.three = 1;
			ssbo_CPUMEM.background = vec4(0);
			true_num_scene_objects = 6;
			myscene = scene6;
		}

		// toggle fullscreen aspect ratio
		if (key == GLFW_KEY_F && action == GLFW_PRESS)
		{
			mycam.f = !mycam.f;
			if (!mycam.f)
				aspect_ratio = ASPECT_RATIO;
			else
				aspect_ratio = FULLSCREEN_ASPECT_RATIO;
		}
	}

	// callback for the mouse when clicked move the triangle when helper functions
	// written
	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		// double posX, posY;
		// float newPt[2];
		// if (action == GLFW_PRESS)
		// {
			
		// }
	}

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}
        void initGeom()
        {
            string resourceDirectory = "../resources";

            // screen plane
            glGenVertexArrays(1, &VertexArrayIDScreen);
            glBindVertexArray(VertexArrayIDScreen);
            // generate vertex buffer to hand off to OGL
            glGenBuffers(1, &VertexBufferIDScreen);
            // set the current state to focus on our vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, VertexBufferIDScreen);
            vec3 vertices[6];
            vertices[0] = vec3(-1, -1, 0);
            vertices[1] = vec3(1, -1, 0);
            vertices[2] = vec3(1, 1, 0);
            vertices[3] = vec3(-1, -1, 0);
            vertices[4] = vec3(1, 1, 0);
            vertices[5] = vec3(-1, 1, 0);
            // actually memcopy the data - only do this once
            glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vec3), vertices,
                         GL_STATIC_DRAW);
            // we need to set up the vertex array
            glEnableVertexAttribArray(0);
            // key function to get up how many elements to pull out at a time
            // (3)
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
            // generate vertex buffer to hand off to OGL
            glGenBuffers(1, &VertexBufferTexScreen);
            // set the current state to focus on our vertex buffer
            glBindBuffer(GL_ARRAY_BUFFER, VertexBufferTexScreen);
            vec2 texscreen[6];
            texscreen[0] = vec2(0, 0);
            texscreen[1] = vec2(1, 0);
            texscreen[2] = vec2(1, 1);
            texscreen[3] = vec2(0, 0);
            texscreen[4] = vec2(1, 1);
            texscreen[5] = vec2(0, 1);
            // actually memcopy the data - only do this once
            glBufferData(GL_ARRAY_BUFFER, 6 * sizeof(vec2), texscreen,
                         GL_STATIC_DRAW);
            // we need to set up the vertex array
            glEnableVertexAttribArray(1);
            // key function to get up how many elements to pull out at a time
            // (3)
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
            glBindVertexArray(0);

            int width, height;

            // shader:
            GLuint Tex1Location;

            Tex1Location = glGetUniformLocation(
                prog->pid,
                "tex");  // tex, tex2... sampler in the fragment shader
            glUseProgram(prog->pid);
            glUniform1i(Tex1Location, 0);

            glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
            // RGBA8 2D texture, 24 bit depth texture, 256x256
            //-------------------------
            // Does the GPU support current FBO configuration?
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // make a texture (buffer) on the GPU to store the input image
            glGenTextures(
                1, &CS_tex_A);  // Generate texture and store context number
            glActiveTexture(
                GL_TEXTURE0);  // since we have 2 textures in this program, we
                               // need to associate the input texture with "0"
                               // meaning first texture
            glBindTexture(GL_TEXTURE_2D, CS_tex_A);  // highlight input texture
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                            GL_CLAMP_TO_EDGE);  // texture sampler parameter
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                            GL_CLAMP_TO_EDGE);  // texture sampler parameter
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                            GL_LINEAR);  // texture sampler parameter
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                            GL_LINEAR);  // texture sampler parameter
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, WIDTH, HEIGHT, 0, GL_RGBA,
                         GL_FLOAT, NULL);  // copy image data to texture
            glBindImageTexture(0, CS_tex_A, 0, GL_FALSE, 0, GL_READ_WRITE,
                               GL_RGBA32F);  // enable texture in shader
        }

    void loadShapeBuffer()
    {
    	// must pack simple shapes into buffer
    	for (int i = 0; i < true_num_scene_objects; i ++)
    	{
    		shape* curr = myscene.shapes[i];
    		if (curr->id() == SPHERE_ID)
    		{
    			vec3 center = ((sphere*) curr)->location;
    			float rad = ((sphere*) curr)->radius;
    			vec3 color = ((sphere*) curr)->color;
    			float reflectivity = curr->reflectivity;
    			float emissive = curr->emissive;
    			int id = SPHERE_ID;

    			// sphere:
    				// vec4: vec3 center, float radius
    				// vec4: vec3 nothing, bool emissive?
    				// vec4: nothing
    				// vec4: vec3 nothing, float reflectivity
    				// vec4: vec3 color, int shape_id

    			ssbo_CPUMEM.simple_shapes[i][0] = vec4(center, rad);
    			ssbo_CPUMEM.simple_shapes[i][1].w = emissive;
    			ssbo_CPUMEM.simple_shapes[i][3].w = reflectivity;
    			ssbo_CPUMEM.simple_shapes[i][4] = vec4(color, id);
    		}
    		if (curr->id() == PLANE_ID)
    		{
    			vec3 normal = ((plane*) curr)->normal;
    			float dist_from_orig = ((plane*) curr)->dist_from_orig;
    			vec3 color = ((plane*) curr)->color;
    			vec3 p0 = ((plane*) curr)->p0;
    			float reflectivity = curr->reflectivity;
    			float emissive = curr->emissive;
    			int id = PLANE_ID;

    			// plane:
    				// vec4: vec3 normal, float distance from origin
    				// vec4: vec3 nothing, bool emissive?
    				// vec4: nothing
    				// vec4: vec3 point in plane, float reflectivity
    				// vec4: vec3 color, int shape_id

    			ssbo_CPUMEM.simple_shapes[i][0] = vec4(normal, dist_from_orig);
    			ssbo_CPUMEM.simple_shapes[i][1].w = emissive;
    			ssbo_CPUMEM.simple_shapes[i][3] = vec4(p0, reflectivity);
    			ssbo_CPUMEM.simple_shapes[i][4] = vec4(color, id);
    		}
			if (curr->id() == RECTANGLE_ID)
			{
				vec3 normal = ((rectangle*) curr)->normal;
				vec3 right = ((rectangle*) curr)->right;
				vec3 up = ((rectangle*) curr)->up;
				vec3 color = ((rectangle*) curr)->color;
				vec3 llc = ((rectangle*) curr)->llv;
				float reflectivity = curr->reflectivity;
				float emissive = curr->emissive;
				int id = RECTANGLE_ID;

				// rectangle:
					// 	vec4: vec3 normal, float nothing
					// 	vec4: vec3 up, bool emmissive?
					// 	vec4: vec3 right, float nothing
					// 	vec4: vec3 lower left corner, float reflectivity
					// 	vec4: vec3 color, int shape_id

				ssbo_CPUMEM.simple_shapes[i][0] = vec4(normal, 0);
				ssbo_CPUMEM.simple_shapes[i][1] = vec4(up, emissive);
				ssbo_CPUMEM.simple_shapes[i][2] = vec4(right, 0);
				ssbo_CPUMEM.simple_shapes[i][3] = vec4(llc, reflectivity);
				ssbo_CPUMEM.simple_shapes[i][4] = vec4(color, id);
			}
		}
	}

	void computeInitGeom()
	{
		std::random_device rd;     // only used once to initialise (seed) engine
		std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
		std::uniform_int_distribution<int> uni(0,4096); // guaranteed unbiased

		// ssbo_CPUMEM.current_time = vec4(glfwGetTime());
		ssbo_CPUMEM.mode = vec4(1,0,0,0);
		ssbo_CPUMEM.horizontal = ssbo_CPUMEM.vertical = vec4();
		ssbo_CPUMEM.llc_minus_campos = ssbo_CPUMEM.camera_location = vec4();
		// maybe there is a better place to store these important default values...
		// instead of buried in computeInitGeom
		ssbo_CPUMEM.background = vec4(13/255.0, 153/255.0, 219/255.0, 0);
		// ssbo_CPUMEM.background = vec4(0);

		for (int i = 0; i < WIDTH; i++)
		{
			for (int j = 0; j < HEIGHT; j++)
			{
				ssbo_CPUMEM.pixels[0][i][j] = vec4();
				ssbo_CPUMEM.pixels[1][i][j] = vec4();
			}
		}

		loadShapeBuffer();

		for (int i = 0; i < AA * 2; i ++)
		{
			ssbo_CPUMEM.rand_buffer[i] = vec4(randf(), randf(), randf(), randf());
		}

		glGenBuffers(1, &ssbo_GPU_id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_data), &ssbo_CPUMEM, GL_DYNAMIC_COPY);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_GPU_id);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
	}

	// //General OGL initialization - set OGL state here
	void computeInit()
	{
		GLSL::checkVersion();
		//load the compute shader
		std::string ShaderString = readFileAsString("../resources/compute.glsl");
		const char *shader = ShaderString.c_str();
		GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(computeShader, 1, &shader, nullptr);

		GLint rc;
		CHECKED_GL_CALL(glCompileShader(computeShader));
		CHECKED_GL_CALL(glGetShaderiv(computeShader, GL_COMPILE_STATUS, &rc));
		if (!rc)	//error compiling the shader file
		{
			GLSL::printShaderInfoLog(computeShader);
			std::cout << "Error compiling compute shader " << std::endl;
			system("pause");
			exit(1);
		}


		computeProgram = glCreateProgram();
		glAttachShader(computeProgram, computeShader);
		glLinkProgram(computeProgram);
		glUseProgram(computeProgram);

		
		GLuint block_index;
		block_index = glGetProgramResourceIndex(computeProgram, GL_SHADER_STORAGE_BLOCK, "shader_data");
		GLuint ssbo_binding_point_index = 0;
		glShaderStorageBlockBinding(computeProgram, block_index, ssbo_binding_point_index);


		ShaderString = readFileAsString("../resources/postprocessing.glsl");
		shader = ShaderString.c_str();
		GLuint postProcessingShader = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(postProcessingShader, 1, &shader, nullptr);


		GLint rcp;
		CHECKED_GL_CALL(glCompileShader(postProcessingShader));
		CHECKED_GL_CALL(glGetShaderiv(postProcessingShader, GL_COMPILE_STATUS, &rcp));
		if (!rcp)	//error compiling the shader file
		{
			GLSL::printShaderInfoLog(postProcessingShader);
			std::cout << "Error compiling post processing shader " << std::endl;
			system("pause");
			exit(1);
		}

		postProcessingProgram = glCreateProgram();
		glAttachShader(postProcessingProgram, postProcessingShader);
		glLinkProgram(postProcessingProgram);
		glUseProgram(postProcessingProgram);

		GLuint block_index2;
		block_index2 = glGetProgramResourceIndex(postProcessingProgram, GL_SHADER_STORAGE_BLOCK, "shader_data");
		ssbo_binding_point_index = 0;
		glShaderStorageBlockBinding(postProcessingProgram, block_index2, ssbo_binding_point_index);
	}

	void compute()
	{
		static int frame_num = 0;
		// TODO use ssbo versions of data so no need to copy
		// copy updated values over... in the future maybe just use the ssbo versions everywhere
		ssbo_CPUMEM.mode.y = frame_num;
		ssbo_CPUMEM.mode.z = true_num_scene_objects;
		ssbo_CPUMEM.horizontal = vec4(horizontal, 0);
		ssbo_CPUMEM.vertical = vec4(vertical, 0);
		ssbo_CPUMEM.llc_minus_campos = vec4(llc_minus_campos, 0);
		ssbo_CPUMEM.camera_location = vec4(rt_camera.location, 0);

		for (int i = 0; i < AA * 2; i ++)
		{
			ssbo_CPUMEM.rand_buffer[i] = vec4(randf(), randf(), randf(), randf());
		}

		GLuint block_index = 1;
		block_index = glGetProgramResourceIndex(computeProgram, GL_SHADER_STORAGE_BLOCK, "shader_data");
		GLuint ssbo_binding_point_index = 0;
		glShaderStorageBlockBinding(computeProgram, block_index, ssbo_binding_point_index);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_GPU_id);
		glUseProgram(computeProgram);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
		GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
		int siz = sizeof(ssbo_data);
		memcpy(p, &ssbo_CPUMEM, siz);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);				

		glDispatchCompute((GLuint) WIDTH, (GLuint) HEIGHT, 1);		//start compute shader
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		GLuint block_index2;
		block_index2 = 1;
		block_index2 = glGetProgramResourceIndex(postProcessingProgram, GL_SHADER_STORAGE_BLOCK, "shader_data");
		GLuint ssbo_binding_point_index2 = 0;
		glShaderStorageBlockBinding(postProcessingProgram, block_index2, ssbo_binding_point_index2);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_GPU_id);
		glUseProgram(postProcessingProgram);

		glDispatchCompute((GLuint)WIDTH, (GLuint)HEIGHT, 1);		//start compute shader
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glBindImageTexture(0, CS_tex_A, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
		
		//copy data back to CPU MEM

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
		p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
		siz = sizeof(ssbo_data);
		memcpy(&ssbo_CPUMEM,p, siz);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		frame_num = (frame_num + 1) % NUM_FRAMES;
	}

	//General OGL initialization - set OGL state here
	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program.
		prog = std::make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/shader_vertex.glsl", resourceDirectory + "/shader_fragment.glsl");
		if (!prog->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("campos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex");

		// Initialize the GLSL program.
		heightshader = std::make_shared<Program>();
		heightshader->setVerbose(true);
		heightshader->setShaderNames(resourceDirectory + "/height_vertex.glsl", resourceDirectory + "/height_frag.glsl");
		if (!heightshader->init())
		{
			std::cerr << "One or more shaders failed to compile... exiting!" << std::endl;
			exit(1);
		}
		heightshader->addUniform("P");
		heightshader->addUniform("V");
		heightshader->addUniform("M");
		heightshader->addAttribute("vertPos");
		heightshader->addAttribute("vertTex");
	}

	void update_camera(double frametime) {
		float rot_y = 0.0;
		float rot_x = 0.0;
		bool rotate = false;
		bool rotate_up_down = false;
		// I wonder if it would be better to store this value in the camera, and update when needed?
		vec3 right = normalize(cross(rt_camera.up, rt_camera.look_towards));
		if (mycam.w == 1)
			rt_camera.location -= 5.0 * rt_camera.look_towards * frametime;
		if (mycam.s == 1)
			rt_camera.location += 5.0 * rt_camera.look_towards * frametime;
		if (mycam.a == 1)
			rt_camera.location -= 5.0 * right * frametime;
		if (mycam.d == 1)
			rt_camera.location += 5.0 * right * frametime;

		if (mycam.sp == 1)
			rt_camera.location += 5.0 * rt_camera.up * frametime;
		if (mycam.ls == 1)
			rt_camera.location -= 5.0 * rt_camera.up * frametime;

		if (mycam.e == 1)
		{
			rot_y += 1.5 * frametime;
			rotate = true;
		}
			
		if (mycam.q == 1)
		{
			rot_y -= 1.5 * frametime;
			rotate = true;
		}

		if (mycam.z == 1)
		{
			rot_x -= 1.5 * frametime;
			rotate_up_down = true;
		}
		if (mycam.c == 1)
		{
			rot_x += 1.5 * frametime;
			rotate_up_down = true;
		}

		if (rotate) {
			glm::mat4 R = glm::rotate(glm::mat4(1), rot_y, rt_camera.up);
			glm::vec4 dir = vec4(rt_camera.look_towards, 0);
			dir = dir * R;
			rt_camera.look_towards = vec3(dir.x, dir.y, dir.z);
			rt_camera.up = vec4(rt_camera.up, 0) * R;
		}

		if (rotate_up_down)
		{
			glm::mat4 R = glm::rotate(glm::mat4(1), rot_x, right);
			glm::vec4 dir = vec4(rt_camera.look_towards, 0);
			dir = dir * R;
			rt_camera.look_towards = vec3(dir.x, dir.y, dir.z);
			rt_camera.up = vec4(rt_camera.up, 0) * R;
		}
	}

	void render()
	{
		double frametime = get_last_elapsed_time();
		cout << "\r" << "framerate: " << int(1/frametime) << "          " << flush;

		loadShapeBuffer();

		update_camera(frametime);

		w = rt_camera.look_towards;
		u = normalize(cross(rt_camera.up, w));
		v = normalize(cross(w, u));

		horizontal = aspect_ratio * u;
		vertical = VERT_ASPECT_RATIO * v;

		llc_minus_campos = -0.5 * (horizontal + vertical) - w;

		compute(); // do the ray tracing here

		// we want to render this to a texture
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width,
                                &height);
        glViewport(0, 0, width, height);
        // Clear framebuffer.
        glClearColor(1.0f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        prog->bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, CS_tex_A);

        glBindVertexArray(VertexArrayIDScreen);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        prog->unbind();
	}

};
//******************************************************************************************
int main(int argc, char **argv)
{
	std::string resourceDir = "../resources"; // Where the resources are loaded from
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	srand(time(0));

	glfwInit();
	GLFWwindow* window = glfwCreateWindow(32, 32, "Dummy", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	gladLoadGL();

	int work_grp_cnt[3];

	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
	glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

	printf("max global (total) work group size x:%i y:%i z:%i\n",
		work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	 WindowManager * windowManager = new WindowManager();
	 windowManager->init(WIDTH, HEIGHT);
	 windowManager->setEventCallbacks(application);
	 application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
	// Initialize scene.
	application->init(resourceDir);
	application->initGeom();
	application->computeInit();
	application->computeInitGeom();

	while (!glfwWindowShouldClose(windowManager->getHandle()))
    {
        application->render();

        // Swap front and back buffers.
        glfwSwapBuffers(windowManager->getHandle());
        // Poll for and process events.
        glfwPollEvents();
    }

	// Quit program.
	windowManager->shutdown();
	cout << endl;
	return 0;
}
