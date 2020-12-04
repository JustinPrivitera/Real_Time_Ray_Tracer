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
#include "scene.h"

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
#define AA 4 // multisample anti-aliasing

// number of scene objects
#define NUM_SHAPES 10

#define NUM_FRAMES 8

// aspect ratio constants
#define ASPECT_RATIO 1.333333 // horizontal
#define FULLSCREEN_ASPECT_RATIO 1.777777 // horizontal
#define VERT_ASPECT_RATIO 1.0

// background colors
#define SKY vec4(13/255.0, 153/255.0, 219/255.0, 0);
#define BLACK vec4(0); 

class ssbo_data
{
public:
	vec4 mode; // utility
	vec4 horizontal; // ray casting vector
	vec4 vertical; // ray casting vector
	vec4 llc_minus_campos; // ray casting vector
	vec4 camera_location; // ray casting vector
	vec4 light_pos; // for point lights only
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

// build ray trace camera
vec3 location = vec3(0,0,14);
vec3 up = vec3(0,1,0);
vec3 look_towards = vec3(0,0,1);
camera mycam = camera(location, up, look_towards);
// end

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

class Application : public EventCallbacks
{

public:

	float aspect_ratio = ASPECT_RATIO;
	int true_num_scene_objects = 5; // NUM_SHAPES;
	int light_movement = 0;

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

	WindowManager * windowManager = nullptr;

	ssbo_data ssbo_CPUMEM;
	GLuint ssbo_GPU_id;
	GLuint aop_computeProgram, aop_postProcessingProgram, ao_computeProgram, p_computeProgram, h_computeProgram;

	// Our shader program
	std::shared_ptr<Program> prog, heightshader;
	// Contains vertex information for OpenGL
	GLuint VertexArrayID, VertexArrayIDScreen;

	GLuint VertexBufferID, VertexBufferTexScreen, VertexBufferIDScreen,VertexNormDBox, VertexTexBox, IndexBufferIDBox, InstanceBuffer;

	//texture data
	GLuint tex;
	int tex_w, tex_h;

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
		if (key == GLFW_KEY_V && action == GLFW_PRESS)
		{
			mycam.v = (mycam.v + 1) % 3;
			if (mycam.v == 0)
			{
				ssbo_CPUMEM.background = SKY;
				myscene = scene1;
				true_num_scene_objects = myscene.shapes.size();
				
			}
			else if (mycam.v == 1)
			{
				ssbo_CPUMEM.background = BLACK;
				myscene = scene5;
				true_num_scene_objects = myscene.shapes.size();
			}
			else if (mycam.v == 2)
			{
				ssbo_CPUMEM.background = BLACK;
				myscene = scene6;
				true_num_scene_objects = myscene.shapes.size();
			}
			else
			{
				cerr << "incorrect scene specified" << endl;
			}
		}

		// toggle light movement
		if (key == GLFW_KEY_L && action == GLFW_PRESS)
		{
			light_movement = !light_movement;
			if (light_movement)
				ssbo_CPUMEM.light_pos = vec4(-12, 8, 7, 0);
		}

		// toggle lighting algorithm
		if (key == GLFW_KEY_1 && action == GLFW_PRESS)
		{
			mycam.lighting = 1;
		}

		// toggle lighting algorithm
		if (key == GLFW_KEY_2 && action == GLFW_PRESS)
		{
			mycam.lighting = 2;
		}

		// toggle lighting algorithm
		if (key == GLFW_KEY_3 && action == GLFW_PRESS)
		{
			mycam.lighting = 3;
		}

		// toggle lighting algorithm
		if (key == GLFW_KEY_4 && action == GLFW_PRESS)
		{
			mycam.lighting = 4;
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
    	mycam.lighting = 1;

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
            1, &tex);  // Generate texture and store context number
        glActiveTexture(
            GL_TEXTURE0);  // since we have 2 textures in this program, we
                           // need to associate the input texture with "0"
                           // meaning first texture
        glBindTexture(GL_TEXTURE_2D, tex);  // highlight input texture
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
        glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_READ_WRITE,
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
		ssbo_CPUMEM.background = SKY;
		// ssbo_CPUMEM.mode = vec4(1,0,0,0);
		// ssbo_CPUMEM.horizontal = ssbo_CPUMEM.vertical = vec4();
		// ssbo_CPUMEM.llc_minus_campos = ssbo_CPUMEM.camera_location = vec4();
		// maybe there is a better place to store these important default values...
		// instead of buried in computeInitGeom
		
		// ssbo_CPUMEM.background = vec4(0);
		ssbo_CPUMEM.light_pos = vec4(-12, 8, 7, 0);

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
		// setup ambient occlusion with post processing shader
		aop_computeProgram = prep_shader_program("aop_compute.glsl");

		// setup post processing shader for ambient occlusion
		aop_postProcessingProgram = prep_shader_program("aop_postprocessing.glsl");

		// setup ambient occlusion without post processing shader
		ao_computeProgram = prep_shader_program("ao_compute.glsl");

		// setup phong shader
		p_computeProgram = prep_shader_program("p_compute.glsl");

		// setup phong + reflections shader
		h_computeProgram = prep_shader_program("h_compute.glsl");
	}

	GLuint prep_shader_program(string filename)
	{
		string ShaderString = readFileAsString("../resources/" + filename);
		const char *shader_source = ShaderString.c_str();
		GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
		glShaderSource(shader, 1, &shader_source, nullptr);

		GLint rc;
		CHECKED_GL_CALL(glCompileShader(shader));
		CHECKED_GL_CALL(glGetShaderiv(shader, GL_COMPILE_STATUS, &rc));
		if (!rc)	//error compiling the shader file
		{
			GLSL::printShaderInfoLog(shader);
			std::cout << "Error compiling " + filename << std::endl;
			system("pause");
			exit(1);
		}

		GLuint computeProgram = glCreateProgram();
		glAttachShader(computeProgram, shader);
		glLinkProgram(computeProgram);
		glUseProgram(computeProgram);

		GLuint block_index;
		block_index = glGetProgramResourceIndex(computeProgram, GL_SHADER_STORAGE_BLOCK, "shader_data");
		GLuint ssbo_binding_point_index = 0;
		glShaderStorageBlockBinding(computeProgram, block_index, ssbo_binding_point_index);
	
		return computeProgram;
	}

	void fill_rand_buffer()
	{
		for (int i = 0; i < AA * 2; i ++)
			ssbo_CPUMEM.rand_buffer[i] = vec4(randf(), randf(), randf(), randf());
	}

	void compute()
	{
		static int frame_num = 0;
		if (mycam.lighting == 1)
		{
			fill_rand_buffer();
			frame_num = compute_two_shaders(frame_num, aop_computeProgram, aop_postProcessingProgram);
		}
		else if (mycam.lighting == 2)
		{
			fill_rand_buffer();
			frame_num = compute_one_shader(frame_num, ao_computeProgram);
		}
		else if (mycam.lighting == 3)
			frame_num = compute_one_shader(frame_num, p_computeProgram);
		else if (mycam.lighting == 4)
			frame_num = compute_one_shader(frame_num, h_computeProgram);
		else
			cerr << "not yet implemented" << endl;
	}

	int compute_one_shader(int frame_num, GLuint computeProgram)
	{
		if (light_movement)
		{
			ssbo_CPUMEM.light_pos = ssbo_CPUMEM.light_pos + vec4(0.1);
			if (ssbo_CPUMEM.light_pos.x > 50)
				ssbo_CPUMEM.light_pos = vec4(-50, 20, -50, 0);
		}
		else
			ssbo_CPUMEM.light_pos = vec4(-4, 10, 20, 0);

		// TODO use ssbo versions of data so no need to copy
		// copy updated values over... in the future maybe just use the ssbo versions everywhere
		ssbo_CPUMEM.mode.y = frame_num;
		ssbo_CPUMEM.mode.z = true_num_scene_objects;
		ssbo_CPUMEM.horizontal = vec4(horizontal, 0);
		ssbo_CPUMEM.vertical = vec4(vertical, 0);
		ssbo_CPUMEM.llc_minus_campos = vec4(llc_minus_campos, 0);
		ssbo_CPUMEM.camera_location = vec4(mycam.location, 0);

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

        glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
		
		//copy data back to CPU MEM

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
		p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
		siz = sizeof(ssbo_data);
		memcpy(&ssbo_CPUMEM,p, siz);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		return (frame_num + 1) % NUM_FRAMES;
	}

	int compute_two_shaders(int frame_num, GLuint computeProgram1, GLuint computeProgram2)
	{
		// TODO use ssbo versions of data so no need to copy
		// copy updated values over... in the future maybe just use the ssbo versions everywhere
		ssbo_CPUMEM.mode.y = frame_num;
		ssbo_CPUMEM.mode.z = true_num_scene_objects;
		ssbo_CPUMEM.horizontal = vec4(horizontal, 0);
		ssbo_CPUMEM.vertical = vec4(vertical, 0);
		ssbo_CPUMEM.llc_minus_campos = vec4(llc_minus_campos, 0);
		ssbo_CPUMEM.camera_location = vec4(mycam.location, 0);

		GLuint block_index;
		block_index = glGetProgramResourceIndex(computeProgram1, GL_SHADER_STORAGE_BLOCK, "shader_data");
		GLuint ssbo_binding_point_index = 0;
		glShaderStorageBlockBinding(computeProgram1, block_index, ssbo_binding_point_index);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_GPU_id);
		glUseProgram(computeProgram1);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
		GLvoid* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
		int siz = sizeof(ssbo_data);
		memcpy(p, &ssbo_CPUMEM, siz);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);				

		glDispatchCompute((GLuint) WIDTH, (GLuint) HEIGHT, 1);		//start compute shader
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		block_index = glGetProgramResourceIndex(computeProgram2, GL_SHADER_STORAGE_BLOCK, "shader_data");
		ssbo_binding_point_index = 0;
		glShaderStorageBlockBinding(computeProgram2, block_index, ssbo_binding_point_index);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_GPU_id);
		glUseProgram(computeProgram2);

		glDispatchCompute((GLuint)WIDTH, (GLuint)HEIGHT, 1);		//start compute shader
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glBindImageTexture(0, tex, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
		
		//copy data back to CPU MEM

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
		p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
		siz = sizeof(ssbo_data);
		memcpy(&ssbo_CPUMEM,p, siz);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		return (frame_num + 1) % NUM_FRAMES;
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
			std::cerr << "One or more shaders failed to compile" << std::endl;
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
			std::cerr << "One or more shaders failed to compile" << std::endl;
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
		vec3 right = normalize(cross(mycam.up, mycam.look_towards));
		if (mycam.w == 1)
			mycam.location -= 5.0 * mycam.look_towards * frametime;
		if (mycam.s == 1)
			mycam.location += 5.0 * mycam.look_towards * frametime;
		if (mycam.a == 1)
			mycam.location -= 5.0 * right * frametime;
		if (mycam.d == 1)
			mycam.location += 5.0 * right * frametime;

		if (mycam.sp == 1)
			mycam.location += 5.0 * mycam.up * frametime;
		if (mycam.ls == 1)
			mycam.location -= 5.0 * mycam.up * frametime;

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
			glm::mat4 R = glm::rotate(glm::mat4(1), rot_y, mycam.up);
			glm::vec4 dir = vec4(mycam.look_towards, 0);
			dir = dir * R;
			mycam.look_towards = vec3(dir.x, dir.y, dir.z);
			mycam.up = vec4(mycam.up, 0) * R;
		}

		if (rotate_up_down)
		{
			glm::mat4 R = glm::rotate(glm::mat4(1), rot_x, right);
			glm::vec4 dir = vec4(mycam.look_towards, 0);
			dir = dir * R;
			mycam.look_towards = vec3(dir.x, dir.y, dir.z);
			mycam.up = vec4(mycam.up, 0) * R;
		}
	}

	void render()
	{
		double frametime = get_last_elapsed_time();
		cout << "\r" << "framerate: " << int(1/frametime) << "          " << flush;

		loadShapeBuffer();

		update_camera(frametime);

		w = mycam.look_towards;
		u = normalize(cross(mycam.up, w));
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
        glBindTexture(GL_TEXTURE_2D, tex);

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

	 WindowManager * windowManager = new WindowManager();
	 windowManager->init(WIDTH, HEIGHT);
	 windowManager->setEventCallbacks(application);
	 application->windowManager = windowManager;

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
