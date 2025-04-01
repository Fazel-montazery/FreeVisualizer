#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>

#include "defs.h"

// App State
struct
{
	// Window
	GLFWwindow* window;
	int32_t winWidth;
	int32_t winHeight;
	int32_t winPosX;
	int32_t winPosY;
	bool fullscreen;
} state = DEFAULT_STATE;

// Blueprints
static bool initWindow(int32_t width, int32_t height);
static void loop(void);
static void deinitApp(void);

// Main
int main(void)
{
	if(!initWindow(state.winWidth, state.winHeight)) return -1;
	loop();
	deinitApp();
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

void glfwWindowPosCallback(GLFWwindow* window, int xpos, int ypos) 
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
	

	state.window = glfwCreateWindow(width, height, "FreeVisualizer", NULL, NULL);
	if (!state.window)
		return false;

	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	if (!primary)
		return false;

	const GLFWvidmode* mode = glfwGetVideoMode(primary);
	if (mode) {
		int xpos = (mode->width - state.winWidth) / 2;
		int ypos = (mode->height - state.winHeight) / 2;
		glfwSetWindowPos(state.window, xpos, ypos);
        }

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

// Main drawing loop
static void loop(void)
{
	while (!glfwWindowShouldClose(state.window))
	{
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glfwSwapBuffers(state.window);
		glfwPollEvents();
	}
}

// Release all resources
static void deinitApp(void)
{
	glfwDestroyWindow(state.window);
	glfwTerminate();
}
