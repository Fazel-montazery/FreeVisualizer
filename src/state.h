#pragma once

#include <GLFW/glfw3.h>

#include "defs.h"

// App State
typedef struct
{
	// General
	bool		testMode;
	double		currentTime;
	double		prevCursorTime;
	bool		isCursorHidden;
	bool		isCursorInside;
	bool		renderSub;
	SrtHandle	srtHandle; // Only valid when renderSub == true

	// Window
	GLFWwindow*	window;
	int32_t		winWidth; // not in fullscreen mode
	int32_t		winHeight; // not in fullscreen mode
	int32_t		winPosX;
	int32_t		winPosY;
	bool		fullscreen;

	// Music
	char*		musicPath;
	thrd_t		musicThread;
	mtx_t		pauseMX;
	cnd_t		pauseCV;
	bool		isPaused;
	int		duration;
	_Atomic int	positionSec;
	_Atomic int	seekNow;
	_Atomic float	peakAmp;
	_Atomic float	avgAmp;
	_Atomic int	ampScale;

	// Shaders
	char		fragShaderPath[PATH_SIZE];
	GLuint		vertShader;
	GLuint		fragShader;
	GLuint		shaderProgram;
	GLint		uniformLocResolution;
	GLint		uniformLocMouse;
	GLint		uniformLocTime;
	GLint		uniformLocPeakAmp;
	GLint		uniformLocAvgAmp;
	GLint		uniformLocColor1;
	GLint		uniformLocColor2;
	GLint		uniformLocColor3;
	GLint		uniformLocColor4;
	vec3		colors[NUM_COLORS];
} State;
