#pragma once

#include <GLFW/glfw3.h>
#include <sndfile.h>
#include <pipewire/pipewire.h>
#include <spa/utils/ringbuffer.h>
#include <spa/param/audio/format-utils.h>

#include "defs.h"

// App State
typedef struct
{
	// General
	bool			testMode;
	double			currentTime;
	double			prevCursorTime;
	bool			isCursorHidden;
	bool			isCursorInside;
	bool			renderSub;
	SrtHandle		srtHandle; // Only valid when renderSub == true

	// Window
	GLFWwindow*		window;
	int32_t			winWidth; // not in fullscreen mode
	int32_t			winHeight; // not in fullscreen mode
	int32_t			winPosX;
	int32_t			winPosY;
	bool			fullscreen;

	// Music
	char*			musicPath;
	SNDFILE*		sndfile;
	SF_INFO			sndinfo;
	double			sndscale; // to normalize for the subformat
	thrd_t			audioProducerThread;
	struct pw_thread_loop*	pwThreadLoop;
	struct pw_loop*		pwLoop;
	struct pw_stream*	pwStream;
	struct pw_properties*	pwProps;
	uint8_t			spaPodbuffer[1024];
	struct spa_pod_builder	spaPodBuilder;
	const struct spa_pod*	spaParams[1];
	int			spaEventFd;
	struct spa_ringbuffer	spaRing;
	float*			ringBuffer;
	bool			isPaused;
	int			duration;
	_Atomic int		positionSec;
	_Atomic int		seekNow;
	_Atomic float		peakAmp;
	_Atomic float		avgAmp;
	_Atomic int		ampScale;

	// Shaders
	char			fragShaderPath[PATH_SIZE];
	GLuint			vertShader;
	GLuint			fragShader;
	GLuint			shaderProgram;
	GLint			uniformLocResolution;
	GLint			uniformLocMouse;
	GLint			uniformLocTime;
	GLint			uniformLocPeakAmp;
	GLint			uniformLocAvgAmp;
	GLint			uniformLocColor1;
	GLint			uniformLocColor2;
	GLint			uniformLocColor3;
	GLint			uniformLocColor4;
	vec3			colors[NUM_COLORS];
	char			savedColorsPath[PATH_SIZE];
	vec3			savedColors[NUM_SAVED_COLORS][NUM_COLORS];
} State;
