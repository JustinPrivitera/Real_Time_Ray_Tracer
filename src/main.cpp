/*
CPE/CSC 471 Lab base code Wood/Dunn/Eckhardt
*/

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

#include "WindowManager.h"
#include "Shape.h"
// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace std;
using namespace glm;
shared_ptr<Shape> shape;

#define WIDTH 10
#define HEIGHT 10

#define NUM_BALLS 100

class ssbo_data
{
public:
	vec3 w;
	vec3 u;
	vec3 v;
	vec3 horizontal;
	vec3 vertical;
	vec3 llc;
	vec3 pixels[WIDTH][HEIGHT];
};


double get_last_elapsed_time()
{
	static double lasttime = glfwGetTime();
	double actualtime =glfwGetTime();
	double difference = actualtime- lasttime;
	lasttime = actualtime;
	return difference;
}
class camera
{
public:
	glm::vec3 pos, rot;
	int w, a, s, d;
	camera()
	{
		w = a = s = d = 0;
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
camera mycam;
#define ACC vec3(0,-9.81,0)
class ball
{
public:
	vec3 pos, v;
	float m;
	float r;
	ball() 
	{
		pos = vec3(0);
		v = vec3(0);
		m = 1;
		r = 0.3;
	}
	void update(float delta_t)
	{
		v = v + ACC*0.001f;
		// if (v.y < 0 && pos.y - r < -4.99 && pos.y - r > -5.01)
		// 	pos = pos;
		// else
			pos = pos + v * delta_t;
		// pos = initpos;
	}
};


float randf()
{
	return (float)(rand() / (float)RAND_MAX);
}

class Application : public EventCallbacks
{

public:

	ball plutos[NUM_BALLS];
	// ball pluto;

	WindowManager * windowManager = nullptr;

	ssbo_data ssbo_CPUMEM;
	GLuint ssbo_GPU_id;
	GLuint computeProgram;

	// Our shader program
	std::shared_ptr<Program> prog, heightshader;

	// Contains vertex information for OpenGL
	GLuint VertexArrayID;

	// Data necessary to give our box to OpenGL
	GLuint MeshPosID, MeshTexID, IndexBufferIDBox;

	//texture data
	GLuint Texture;
	GLuint Texture2,HeightTex;

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
	}

	// callback for the mouse when clicked move the triangle when helper functions
	// written
	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;
		float newPt[2];
		if (action == GLFW_PRESS)
		{
			
		}
	}

	//if the window is resized, capture the new size and reset the viewport
	void resizeCallback(GLFWwindow *window, int in_width, int in_height)
	{
		//get the window size - may be different then pixels for retina
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
	}
#define MESHSIZE 4
	void init_mesh()
	{
		//generate the VAO
		glGenVertexArrays(1, &VertexArrayID);
		glBindVertexArray(VertexArrayID);

		//generate vertex buffer to hand off to OGL
		glGenBuffers(1, &MeshPosID);
		glBindBuffer(GL_ARRAY_BUFFER, MeshPosID);
		vec3 vertices[MESHSIZE];
		
		vertices[0] = vec3(1.0, 0.0, 0.0);
		vertices[1] = vec3(0.0, 0.0, 0.0);
		vertices[2] = vec3(0.0, 0.0, 1.0);
		vertices[3] = vec3(1.0, 0.0, 1.0);
		

		glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) *MESHSIZE, vertices, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
		//tex coords
		float t = 1. / 100;
		vec2 tex[MESHSIZE];
		tex[0] = vec2(1.0, 0.0);
		tex[1] = vec2(0,  0.0);
		tex[2] = vec2(0,  1.0);
		tex[3] = vec2(1.0, 1.0);

		glGenBuffers(1, &MeshTexID);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, MeshTexID);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * MESHSIZE, tex, GL_STATIC_DRAW);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		glGenBuffers(1, &IndexBufferIDBox);
		//set the current state to focus on our vertex buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
		GLushort elements[6];
		int ind = 0;
		
		elements[0] = 0;
		elements[1] = 1;
		elements[2] = 2;
		elements[3] = 0;
		elements[4] = 2;
		elements[5] = 3;
				
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort) * 6, elements, GL_STATIC_DRAW);
		glBindVertexArray(0);
	}
	/*Note that any gl calls must always happen after a GL state is initialized */
	void initGeom()
	{
		//initialize the net mesh
		init_mesh();

		string resourceDirectory = "../resources" ;
		// Initialize mesh.
		shape = make_shared<Shape>();
		//shape->loadMesh(resourceDirectory + "/t800.obj");
		shape->loadMesh(resourceDirectory + "/sphere.obj");
		shape->resize();
		shape->init();

		int width, height, channels;
		char filepath[1000];

		//texture 1
		string str = resourceDirectory + "/grid.jpg";
		strcpy(filepath, str.c_str());
		unsigned char* data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		//texture 2
		str = resourceDirectory + "/sky.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &Texture2);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Texture2);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		//texture 3
		str = resourceDirectory + "/pluto.jpg";
		strcpy(filepath, str.c_str());
		data = stbi_load(filepath, &width, &height, &channels, 4);
		glGenTextures(1, &HeightTex);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, HeightTex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);


		//[TWOTEXTURES]
		//set the 2 textures to the correct samplers in the fragment shader:
		GLuint Tex1Location = glGetUniformLocation(prog->pid, "tex");//tex, tex2... sampler in the fragment shader
		GLuint Tex2Location = glGetUniformLocation(prog->pid, "tex2");
		// Then bind the uniform samplers to texture units:
		glUseProgram(prog->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);

		Tex1Location = glGetUniformLocation(heightshader->pid, "tex");//tex, tex2... sampler in the fragment shader
		Tex2Location = glGetUniformLocation(heightshader->pid, "tex2");
		// Then bind the uniform samplers to texture units:
		glUseProgram(heightshader->pid);
		glUniform1i(Tex1Location, 0);
		glUniform1i(Tex2Location, 1);


		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// make the positions not randomized

		vector<vec3> positions;

		for (int z = -16; z >= -24; z --)
			for (int y = 4; y >= -4; y --)
				for (int x = 4; x >= -4; x --)
					positions.push_back(vec3(x,y,z));

		if (positions.size() < NUM_BALLS)
		{
			cout << "very very bad" << endl;
		}

		for (int i = 0; i < NUM_BALLS; i ++)
		{
			plutos[i].pos = positions[i];
			// plutos[i].pos = vec3(6 * (randf() - 0.5),6 * (randf() - 0.5),-20 + 6 * (randf() - 0.5));
			plutos[i].v = vec3(6 * (randf() - 0.5), 6 * (randf() - 0.5), 6 * (randf() - 0.5));
			// plutos[i].v = vec3(-7, 6 * (randf() - 0.5), 6);
		}
	}

	// void computeInitGeom()
	// {
	// 	std::random_device rd;     // only used once to initialise (seed) engine
	// 	std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
	// 	std::uniform_int_distribution<int> uni(0,4096); // guaranteed unbiased
	
	// 	//make an SSBO
	// 	for (int i = 0; i < NUM_BALLS; i++)
	// 	{
	// 		// ssbo_CPUMEM.dataA[i] = vec4(0.0,0.0,0.0,0.0);
	// 		// ssbo_CPUMEM.dataB[i] = vec4(0.0,0.0,0.0,0.0);

	// 		ssbo_CPUMEM.dataA[i] = vec4(plutos[i].pos.x, plutos[i].pos.y, plutos[i].pos.z, plutos[i].r);
	// 		ssbo_CPUMEM.dataB[i] = vec4(plutos[i].v.x, plutos[i].v.y, plutos[i].v.z, plutos[i].m);
	// 		ssbo_CPUMEM.dataC[i] = vec4(plutos[i].v.x, plutos[i].v.y, plutos[i].v.z, plutos[i].m);
	// 		// cout << ssbo_CPUMEM.dataA[i].x << endl;
	// 	}


	// 	glGenBuffers(1, &ssbo_GPU_id);
	// 	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
	// 	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(ssbo_data), &ssbo_CPUMEM, GL_DYNAMIC_COPY);
	// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_GPU_id);
	// 	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // unbind
	// }

	// //General OGL initialization - set OGL state here
	// void computeInit()
	// {
	// 	GLSL::checkVersion();
	// 	//load the compute shader
	// 	std::string ShaderString = readFileAsString("../resources/compute.glsl");
	// 	const char *shader = ShaderString.c_str();
	// 	GLuint computeShader = glCreateShader(GL_COMPUTE_SHADER);
	// 	glShaderSource(computeShader, 1, &shader, nullptr);

	// 	GLint rc;
	// 	CHECKED_GL_CALL(glCompileShader(computeShader));
	// 	CHECKED_GL_CALL(glGetShaderiv(computeShader, GL_COMPILE_STATUS, &rc));
	// 	if (!rc)	//error compiling the shader file
	// 	{
	// 		GLSL::printShaderInfoLog(computeShader);
	// 		std::cout << "Error compiling compute shader " << std::endl;
	// 		exit(1);
	// 	}

	// 	computeProgram = glCreateProgram();
	// 	glAttachShader(computeProgram, computeShader);
	// 	glLinkProgram(computeProgram);
	// 	glUseProgram(computeProgram);
		
	// 	GLuint block_index;
	// 	block_index = glGetProgramResourceIndex(computeProgram, GL_SHADER_STORAGE_BLOCK, "shader_data");
	// 	GLuint ssbo_binding_point_index = 2;
	// 	glShaderStorageBlockBinding(computeProgram, block_index, ssbo_binding_point_index);

	// }
	// void compute()
	// {
	// 	//make an SSBO
	// 	for (int i = 0; i < NUM_BALLS; i++)
	// 	{
	// 		ssbo_CPUMEM.dataA[i] = vec4(plutos[i].pos.x, plutos[i].pos.y, plutos[i].pos.z, plutos[i].r);
	// 		ssbo_CPUMEM.dataB[i] = vec4(plutos[i].v.x, plutos[i].v.y, plutos[i].v.z, plutos[i].m);
	// 		ssbo_CPUMEM.dataC[i] = vec4(plutos[i].v.x, plutos[i].v.y, plutos[i].v.z, plutos[i].m);
	// 		// cout << ssbo_CPUMEM.dataA[i].x << ", " << ssbo_CPUMEM.dataA[i].y << ", " << ssbo_CPUMEM.dataA[i].z << endl;
	// 		// cout << ssbo_CPUMEM.dataA[i].x << endl;
	// 	}

	// 	GLuint block_index = 0;
	// 	block_index = glGetProgramResourceIndex(computeProgram, GL_SHADER_STORAGE_BLOCK, "shader_data");
	// 	GLuint ssbo_binding_point_index = 0;
	// 	glShaderStorageBlockBinding(computeProgram, block_index, ssbo_binding_point_index);
	// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_GPU_id);
	// 	glUseProgram(computeProgram);

	// 	// glUniform1ui(1, b);

	// 	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
	// 	GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
	// 	int siz = sizeof(ssbo_data);
	// 	memcpy(p, &ssbo_CPUMEM, siz);
	// 	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	// 	// what is going on with the memcpys? why is it that i have to use the different sizes otherwise it breaks?
				

	// 	glDispatchCompute((GLuint)1, (GLuint)1, 1);				//start compute shader
	// 	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// 	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
		
	// 	//copy data back to CPU MEM

	// 	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
	// 	p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
	// 	siz = sizeof(ssbo_data);
	// 	memcpy(&ssbo_CPUMEM,p, siz);
	// 	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	// 	for (int i = 0; i < NUM_BALLS; i++)
	// 	{
	// 		plutos[i].v = vec3(ssbo_CPUMEM.dataC[i].x, ssbo_CPUMEM.dataC[i].y, ssbo_CPUMEM.dataC[i].z);
	// 	}
	// }

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

	void update(float dt)
	{
		for (int i = 0; i < NUM_BALLS; i ++)
		{
			plutos[i].update(dt);
		}
	}

	/****DRAW
	This is the most important function in your program - this is where you
	will actually issue the commands to draw any geometry you have set up to
	draw
	********/
	void render()
	{
		// compute();
		double frametime = get_last_elapsed_time();
		update(frametime);

		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		float aspect = width/(float)height;
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClearColor(0.8f, 0.8f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Create the matrix stacks - please leave these alone for now
		
		glm::mat4 V, M, P; //View, Model and Perspective matrix
		mat4 TransZ, S, RotateY, RotateX, TransY;
		V = glm::mat4(1);
		M = glm::mat4(1);
		// Apply orthographic projection....
		P = glm::ortho(-1 * aspect, 1 * aspect, -1.0f, 1.0f, -2.0f, 100.0f);		
		if (width < height)
			{
			P = glm::ortho(-1.0f, 1.0f, -1.0f / aspect,  1.0f / aspect, -2.0f, 100.0f);
			}
		// ...but we overwrite it (optional) with a perspective projection.
		P = glm::perspective((float)(3.14159 / 4.), (float)((float)width/ (float)height), 0.1f, 1000.0f); //so much type casting... GLM metods are quite funny ones

		//animation with the model matrix:
		static float w = 0.0;
		w += 1.0 * frametime;//rotation angle
		float trans = 0;// sin(t) * 2;
		RotateY = glm::rotate(glm::mat4(1.0f), w, glm::vec3(0.0f, 1.0f, 0.0f));
		float angle = -3.1415926/2.0;
		RotateX = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1.0f, 0.0f, 0.0f));

		// Draw the box using GLSL.
		prog->bind();

		V = mycam.process(frametime);
		//send the matrices to the shaders

		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		glUniform3fv(prog->getUniform("campos"), 1, &mycam.pos[0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, HeightTex);

		for (int i = 0; i < NUM_BALLS; i ++)
		{
			TransZ = glm::translate(glm::mat4(1.0f), plutos[i].pos);
			S = glm::scale(glm::mat4(1.0f), glm::vec3(plutos[i].r));
			M =  TransZ * RotateY * RotateX * S;
			glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, &M[0][0]);
			shape->draw(prog,0);
		}

		heightshader->bind();
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		S = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 10.0f, 10.0f));
		TransY = glm::translate(glm::mat4(1.0f), glm::vec3(-5.0f, -5.0f, -25));
		M = TransY * S;
		glUniformMatrix4fv(heightshader->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glUniformMatrix4fv(heightshader->getUniform("P"), 1, GL_FALSE, &P[0][0]);
		glUniformMatrix4fv(heightshader->getUniform("V"), 1, GL_FALSE, &V[0][0]);
		
		glBindVertexArray(VertexArrayID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferIDBox);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, Texture);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);

		M = TransY * S * RotateX;
		glUniformMatrix4fv(heightshader->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);

		RotateY = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
		M = TransY * S * RotateY*RotateX;
		glUniformMatrix4fv(heightshader->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);

		RotateY = glm::rotate(glm::mat4(1.0f), -angle, glm::vec3(0.0f, 1.0f, 0.0f));
		TransY = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, -5.0f, -15));
		M = TransY * S * RotateY * RotateX;
		glUniformMatrix4fv(heightshader->getUniform("M"), 1, GL_FALSE, &M[0][0]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);
		heightshader->unbind();

	}

};
//******************************************************************************************
int main(int argc, char **argv)
{
	///////////////////////////////////////////////////////////

	std::string resourceDir = "../resources"; // Where the resources are loaded from
	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	///////////////////////////////////////////////////////////////
	// ComputeApplication *computeapplication = new ComputeApplication();
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

	// computeapplication->init();
	// computeapplication->initGeom();

	//////////////////////////////////////////////////////////////////////

	/* your main will always include a similar set up to establish your window
		and GL context, etc. */
	WindowManager * windowManager = new WindowManager();
	windowManager->init(1920, 1080);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	/* This is the code that will likely change program to program as you
		may need to initialize or set up different data and state */
	// Initialize scene.
	application->init(resourceDir);
	application->initGeom();
	// application->computeInit();
	// application->computeInitGeom();

	
	// application->computeInitGeom();

	// Loop until the user closes the window.
	while(! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// application->compute();

		// Render scene.
		application->render();

		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
