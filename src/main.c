#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>

#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>

#include "defs.h"
#include "opts.h"
#include "shader.h"

// App State
static struct
{
	// Window
	GLFWwindow* window;
	int32_t winWidth;
	int32_t winHeight;
	int32_t winPosX;
	int32_t winPosY;
	bool fullscreen;

	// Music
	char* musicPath;

	// Shaders
	char fragShaderPath[PATH_SIZE];
	GLuint vertShader;
	GLuint fragShader;
	GLuint shaderProgram;

} state = DEFAULT_STATE;

// Blueprints
static bool initWindow(int32_t width, int32_t height);
static bool initShaders(const char* fragShaderPath);
static void loop(void);
static void deinitApp(int sig);

// Main
int main(int argc, char** argv)
{
	// Setting up the Ctrl+C signal
	signal(SIGINT, deinitApp);

	if (!parseOpts(argc, argv, &state.musicPath, state.fragShaderPath, PATH_SIZE, &state.fullscreen))
		return 0;

	if (!initWindow(state.winWidth, state.winHeight)) return -1;
	if (!initShaders(state.fragShaderPath)) return -1;
	loop();
	deinitApp(0);
	return 0;
}

// Helper functions
#ifndef NDEBUG
static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}
#endif

static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	if (!state.fullscreen) {
		state.winWidth = width;
		state.winHeight = height;
	}
}

static void glfwWindowPosCallback(GLFWwindow* window, int xpos, int ypos) 
{
	if (!state.fullscreen) {
		state.winPosX = xpos;
		state.winPosY = ypos;
	}
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	else if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	else if (key == GLFW_KEY_F && action == GLFW_PRESS) {
		GLFWmonitor* primary = glfwGetPrimaryMonitor();
		if (!primary) return;
		const GLFWvidmode* mode = glfwGetVideoMode(primary);
		if (!mode) return;
		if (state.fullscreen) {
			glfwSetWindowMonitor(state.window, NULL, state.winPosX, state.winPosY,
					state.winWidth,
					state.winHeight,
					0);
			state.fullscreen = false;
		} else {
			glfwSetWindowMonitor(state.window, primary, 0, 0,
					mode->width,
					mode->height,
					mode->refreshRate);
			state.fullscreen = true;
		}
	}
}

// Initialize the window for opengl
static bool initWindow(int32_t width, int32_t height)
{
#ifndef NDEBUG
	glfwSetErrorCallback(glfw_error_callback);
#endif

	if (!glfwInit())
		return false;

#ifdef NDEBUG
	glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);
#endif
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
	glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
	glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
	glfwWindowHint(GLFW_CENTER_CURSOR, GLFW_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 0);
	glfwWindowHint(GLFW_REFRESH_RATE, GLFW_DONT_CARE);
	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	if (!primary)
		return false;

	int xpos, ypos;
	const GLFWvidmode* mode = glfwGetVideoMode(primary);
	if (mode) {
		xpos = (mode->width - state.winWidth) / 2;
		ypos = (mode->height - state.winHeight) / 2;
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        }


	state.window = glfwCreateWindow(width, height, "FreeVisualizer", (state.fullscreen) ? primary : NULL, NULL);
	if (!state.window)
		return false;

	if (mode) glfwSetWindowPos(state.window, xpos, ypos);

	glfwMakeContextCurrent(state.window);
	glfwSetFramebufferSizeCallback(state.window, glfw_framebuffer_size_callback);
	glfwSetWindowPosCallback(state.window, glfwWindowPosCallback);
	glfwSetKeyCallback(state.window, glfw_key_callback);

	// setting v-sync
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		fprintf(stderr, "Failed to initialize GLAD\n");
		return false;
	}

	return true;
}

// Load and compile glsl shaders (for fragment shader it uses the path at state.fragShaderPath)
static bool initShaders(const char* fragShaderPath)
{
	// From: https://stackoverflow.com/a/59739538 this beautiful answer by derhass
	const char *vertexShaderSource = "#version 330 core\n"
		"out vec2 texcoords;\n"
		"void main()\n"
		"{\n"
		"vec2 vertices[3]=vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1, 3));\n"
		"gl_Position = vec4(vertices[gl_VertexID],0,1);\n"
		"texcoords = 0.5 * gl_Position.xy + vec2(0.5);\n"
		"}\0";

	if (!createShaderFromSrc(vertexShaderSource, &state.vertShader, GL_VERTEX_SHADER))
		return false;

	if (!createShaderFromPath(fragShaderPath, &state.fragShader, GL_FRAGMENT_SHADER)) {
		glDeleteShader(state.vertShader);
		return false;
	}

	if (!createProgrm(state.vertShader, state.fragShader, &state.shaderProgram)) {
		glDeleteShader(state.vertShader);
		glDeleteShader(state.fragShader);
		return false;
	}

	glDeleteShader(state.vertShader);
	glDeleteShader(state.fragShader);

	return true;
}

// Main drawing loop
static void loop(void)
{
	GLuint dummyVAO; 
	glGenVertexArrays(1, &dummyVAO);
	glBindVertexArray(dummyVAO);

	while (!glfwWindowShouldClose(state.window))
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(state.shaderProgram);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwSwapBuffers(state.window);
		glfwPollEvents();
	}
}

// Release all resources
static void deinitApp(int sig)
{
	glDeleteProgram(state.shaderProgram);
	glfwDestroyWindow(state.window);
	glfwTerminate();
	// Show terminal cursor (if hidden)
	printf("\033[?25h");
	fflush(stdout);
}
