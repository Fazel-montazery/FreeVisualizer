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
} state = DEFAULT_STATE;

// Blueprints
static bool initWindow(int32_t width, int32_t height);
static void loop(void);
static void deinitApp(void);

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
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	else if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// Initialize the window for opengl
static bool initWindow(int32_t width, int32_t height)
{
#ifndef NDEBUG
	glfwSetErrorCallback(glfw_error_callback);
#endif

	if (!glfwInit())
		return false;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	state.window = glfwCreateWindow(width, height, "FreeVisualizer", NULL, NULL);
	if (!state.window)
		return false;

	glfwMakeContextCurrent(state.window);
	glfwSetFramebufferSizeCallback(state.window, glfw_framebuffer_size_callback);
	glfwSetKeyCallback(state.window, glfw_key_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		fprintf(stderr, "Failed to initialize GLAD\n");
		return false;
	}

	return true;
}

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
