#include <stdio.h>
#include <stdbool.h>

#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>

#include "defs.h"

// Helper functions

#ifndef NDEBUG
static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}
#endif

// Initialize the window for opengl
static bool initWindow(int width, int height)
{
#ifndef NDEBUG
	glfwSetErrorCallback(glfw_error_callback);
#endif

	if (!glfwInit())
	{
		return false;
	}

	return true;
}

// Release all resources
static void deinitApp(void)
{
	glfwTerminate();
}

int main(void)
{
	if(!initWindow(WIN_WIDTH, WIN_HEIGHT)) return -1;
	deinitApp();
	return 0;
}
