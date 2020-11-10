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
#define WIDTH 640
#define HEIGHT 480

// number of scene objects
#define NUM_SHAPES 3

// aspect ratio constants
#define ASPECT_RATIO 1.333333 // horizontal
#define FULLSCREEN_ASPECT_RATIO 1.777777 // horizontal
#define VERT_ASPECT_RATIO 1.0

class ssbo_data
{
public:
	vec4 mode;
	vec4 w;
	vec4 u;
	vec4 v;
	vec4 horizontal;
	vec4 vertical;
	vec4 llc_minus_campos;
	vec4 camera_location;
	vec4 background; // represents the background color
	vec4 light_pos; // for point lights only
	vec4 simple_shapes[NUM_SHAPES][3];
	// sphere: vec4 center, radius; vec4 nothing; vec4 color, shape_id
	// plane: vec4 normal, distance from origin; vec4 point in plane; vec4 color, shape_id
	// vec4 pixels[WIDTH][HEIGHT];
	// vec4 rand_buffer[WIDTH][HEIGHT];
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
	int w, a, s, d, m, f, q, e;
	fake_camera()
	{
		w = a = s = d = m = f = q = e = 0;
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

vec4 pow_vec(vec4 vec, vec4 pows)
{
	return vec4(pow(vec.x, pows.x), pow(vec.y, pows.y), pow(vec.z, pows.z), pow(vec.w, pows.w));
}

vec4 clamp(vec4 v, float min, float max)
{
	return vec4(clamp(v.x, min, max), clamp(v.y, min, max), clamp(v.z, min, max), clamp(v.w, min, max));
}

class scene
{
public:
	scene(vector<shape*> shapes, vector<light_source> lights) : 
		shapes(shapes), lights(lights) {}
	// scene(vector<shape> shapes, vector<light_source> lights, camera cam) : 
	// 	shapes(shapes), lights(lights), cam(cam) {}

public:
	vector<shape*> shapes;
	vector<light_source> lights;
	// camera cam;
};

class Application : public EventCallbacks
{

public:

	float aspect_ratio = ASPECT_RATIO;

	scene myscene = init_scene();

	// copies of the SSBO data since these values will change each frame
	vec3 w;
	vec3 u;
	vec3 v;
	vec3 horizontal;
	vec3 vertical;
	vec3 llc_minus_campos;
	vec3 camera_location;
	// vec3 light_pos;
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
	GLuint computeProgram;

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

	scene init_scene()
	{
		// sphere
		vec3 center = vec3(0,0,0);
		float radius = 2;
		pigment color = pigment(vec3(0.8,0.2,0.5)); // TODO rip out pigments
		sphere* mysphere = new sphere(center,radius,color);

		// sphere
		center = vec3(4,0,-2);
		radius = 3.5;
		color = pigment(vec3(0.8,0.8,0.1));
		sphere* mysphere2 = new sphere(center,radius,color);

		// plane
		vec3 normal = vec3(0, 1, 0);
		float distance_from_origin = -4;
		color = pigment(vec3(0.3,0.0,0.5));
		plane* myplane = new plane(normal, distance_from_origin, color);
		
		// shapes vector
		vector<shape*> myshapes = vector<shape*>();
		myshapes.push_back(mysphere);
		myshapes.push_back(mysphere2);
		myshapes.push_back(myplane);

		if (myshapes.size() != NUM_SHAPES)
			cerr << "num shapes mismatch" << endl;

		// light sources
		vector<light_source> lights = vector<light_source>();

		// make scene
		return scene(myshapes,lights);
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

		// toggle lighting mode
		if (key == GLFW_KEY_M && action == GLFW_PRESS)
		{
			mycam.m = !mycam.m;
			ssbo_CPUMEM.mode.x = mycam.m;
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

            int width, height, channels;
            char filepath[1000];

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


	void computeInitGeom()
	{
		std::random_device rd;     // only used once to initialise (seed) engine
		std::mt19937 rng(rd());    // random-number engine used (Mersenne-Twister in this case)
		std::uniform_int_distribution<int> uni(0,4096); // guaranteed unbiased

		// ssbo_CPUMEM.current_time = vec4(glfwGetTime());
		ssbo_CPUMEM.mode = vec4(0);
		ssbo_CPUMEM.w = ssbo_CPUMEM.u = ssbo_CPUMEM.v = vec4();
		ssbo_CPUMEM.horizontal = ssbo_CPUMEM.vertical = vec4();
		ssbo_CPUMEM.llc_minus_campos = ssbo_CPUMEM.camera_location = vec4();
		// maybe there is a better place to store these important default values...
		// instead of buried in computeInitGeom
		ssbo_CPUMEM.background = vec4(13/255.0, 153/255.0, 219/255.0, 0);
		ssbo_CPUMEM.light_pos = vec4(-12, 8, 7, 0);

		// must pack simple shapes into buffer
		for (int i = 0; i < NUM_SHAPES; i ++)
		{
			shape* curr = myscene.shapes[i];
			if (curr->id() == SPHERE_ID)
			{
				vec3 center = ((sphere*) curr)->location;
				float rad = ((sphere*) curr)->radius;
				vec3 color = ((sphere*) curr)->p.rgb;
				int id = SPHERE_ID;

				ssbo_CPUMEM.simple_shapes[i][0] = vec4(center, rad);
				ssbo_CPUMEM.simple_shapes[i][2] = vec4(color, id);
			}
			if (curr->id() == PLANE_ID)
			{
				vec3 normal = ((plane*) curr)->normal;
				float dist_from_orig = ((plane*) curr)->dist_from_orig;
				vec3 color = ((plane*) curr)->p.rgb;
				vec3 p0 = ((plane*) curr)->p0;
				int id = PLANE_ID;

				ssbo_CPUMEM.simple_shapes[i][0] = vec4(normal, dist_from_orig);
				ssbo_CPUMEM.simple_shapes[i][1] = vec4(p0, 0);
				ssbo_CPUMEM.simple_shapes[i][2] = vec4(color, id);
			}
		}

		// for (int i = 0; i < WIDTH; i ++)
		// {
		// 	for (int j = 0; j < HEIGHT; j ++)
		// 	{
		// 		ssbo_CPUMEM.rand_buffer[i][j] = vec4(randf(), randf(), randf(), 0);
		// 	}
		// }

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
	}

	void compute()
	{
		// TODO use ssbo versions of data so no need to copy
		// copy updated values over... in the future maybe just use the ssbo versions everywhere
		// ssbo_CPUMEM.current_time = vec4(glfwGetTime());
		// ssbo_CPUMEM.mode = vec4(0);
		ssbo_CPUMEM.w = vec4(w, 0);
		ssbo_CPUMEM.u = vec4(u, 0);
		ssbo_CPUMEM.v = vec4(v, 0);
		ssbo_CPUMEM.horizontal = vec4(horizontal, 0);
		ssbo_CPUMEM.vertical = vec4(vertical, 0);
		ssbo_CPUMEM.llc_minus_campos = vec4(llc_minus_campos, 0);
		ssbo_CPUMEM.camera_location = vec4(camera_location, 0);

		// for (int i = 0; i < WIDTH; i ++)
		// {
		// 	for (int j = 0; j < HEIGHT; j ++)
		// 	{
		// 		ssbo_CPUMEM.rand_buffer[i][j] = vec4(randf(), randf(), randf(), 0);
		// 	}
		// }

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
        glBindImageTexture(0, CS_tex_A, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
		
		//copy data back to CPU MEM

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_GPU_id);
		p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);
		siz = sizeof(ssbo_data);
		memcpy(&ssbo_CPUMEM,p, siz);
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
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

	void update_camera() {
		float rot_y = 0.0;
		bool rotate = false;
		vec3 right = normalize(cross(rt_camera.up, rt_camera.look_towards));
		if (mycam.w == 1)
			rt_camera.location -= 0.1 * rt_camera.look_towards;
		if (mycam.s == 1)
			rt_camera.location += 0.1 * rt_camera.look_towards;
		if (mycam.a == 1)
			rt_camera.location -= 0.1 * right;
		if (mycam.d == 1)
			rt_camera.location += 0.1 * right;
		if (mycam.e == 1)
			rot_y += 0.01;
			rotate = true;
		if (mycam.q == 1)
			rot_y -= 0.01;
			rotate = true;

		if (rotate) {
			glm::mat4 R = glm::rotate(glm::mat4(1), rot_y, glm::vec3(0, 1, 0));
			glm::vec4 dir = vec4(rt_camera.look_towards, 0);
			dir = dir * R;
			rt_camera.look_towards = vec3(dir.x, dir.y, dir.z);
		}
	}

	void render()
	{
		update_camera();

		w = rt_camera.look_towards;
		u = normalize(cross(rt_camera.up, w));
		v = normalize(cross(w, u));

		horizontal = aspect_ratio * u;
		vertical = VERT_ASPECT_RATIO * v;

		// llc = rt_camera.location - 0.5 * (horizontal + vertical) - w;

		llc_minus_campos = -0.5 * (horizontal + vertical) - w;

		camera_location = rt_camera.location;

		compute();

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

		double frametime = get_last_elapsed_time();
		cout << "\r" << "framerate: " << int(1/frametime) << "          " << flush;
	}

};
//******************************************************************************************
int main(int argc, char **argv)
{
	//outFile.open("../out.ppm");

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

	//////////////////////////////////////////////////////////////////////

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
        // timef = 1./get_last_elapsed_time();
        // printf("%f\n", timef);
    }

	// Quit program.
	windowManager->shutdown();
	//outFile.close();
	cout << endl;
	return 0;
}
