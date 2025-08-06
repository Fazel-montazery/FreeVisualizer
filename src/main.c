#ifdef __STDC_NO_THREADS__
#error "<threads.h> not supported"
#endif

#include <threads.h>
#include <stdatomic.h>
#include <signal.h>
#include <math.h>

#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>

#include "defs.h"
#include "opts.h"
#include "shader.h"
#include "sound.h"

#include "ft2build.h"
#include FT_FREETYPE_H

// App State
typedef struct
{
	// General
	bool testMode;
	double currentTime;
	double prevCursorTime;
	bool isCursorHidden;
	bool isCursorInside;
	bool renderSub;
	SrtHandle srtHandle; // Only valid when renderSub == true

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

// Blueprints
static bool initWindow(State* state, const int32_t width, const int32_t height);
static bool initShaders(State* state, const char* fragShaderPath);
static bool initAudio(State* state);
static void loop(State* state);
static void deinitApp(State* state);
static void catchCtrlC(int sig);

// Globs
static _Atomic bool isExit;

// Main
int main(int argc, char** argv)
{
	// Hide terminal cursor
	printf("\033[?25l");
	fflush(stdout);

	// Initializing the app state
	State state = { 0 };

	// Initializing atomics
	atomic_store_explicit(&isExit, false, memory_order_relaxed);
	atomic_store_explicit(&state.positionSec, 0, memory_order_relaxed);
	atomic_store_explicit(&state.seekNow, 0, memory_order_relaxed);
	atomic_store_explicit(&state.peakAmp, 0.0, memory_order_relaxed);
	atomic_store_explicit(&state.avgAmp, 0.0, memory_order_relaxed);
	atomic_store_explicit(&state.ampScale, 10, memory_order_relaxed); // 10 * 0.1 = 1.0

	// Setting up the Ctrl+C signal
	signal(SIGINT, catchCtrlC);

	char fragShaderPath[PATH_SIZE];
	if (!parseOpts(argc, argv, 
			&state.musicPath, 
			fragShaderPath, PATH_SIZE, 
			&state.fullscreen, &state.testMode, 
			&state.renderSub, &state.srtHandle,
			state.colors)) {
		return 0;
	}

	if (!initWindow(&state, DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT)) return -1;
	if (!initShaders(&state, fragShaderPath)) return -1;
	if (!state.testMode)
		if (!initAudio(&state)) return -1;
	loop(&state);
	deinitApp(&state);
	return 0;
}

// Helper functions
static int audio_playback_callback(void* arg)
{
	State* state = arg;

	static float buffer [MUSIC_BUFFER_SIZE];

	SF_INFO info = { 0 };
	SNDFILE* sndFile = sf_open(state->musicPath, SFM_READ, &info);

	if (!sndFile) {
		fprintf(stderr, "Error opening file: %s\n", sf_strerror(NULL));
		atomic_store_explicit(&isExit, true, memory_order_relaxed);
		return 0;
	}

	if (info.channels < 1 || info.channels > 2) {	
		fprintf (stderr, "Error : channels = %d\n", info.channels);
		atomic_store_explicit(&isExit, true, memory_order_relaxed);
		return 0;
	}

	snd_pcm_t* pcmHandle = alsa_open(info.channels, (unsigned) info.samplerate, SF_TRUE);
	if (!pcmHandle) {
		atomic_store_explicit(&isExit, true, memory_order_relaxed);
		return 0;
	}
	
	int subformat = info.format & SF_FORMAT_SUBMASK;
	int readcount = 0;

	if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE) {
		double	scale;

		sf_command (sndFile, SFC_CALC_SIGNAL_MAX, &scale, sizeof (scale)) ;
		if (scale > 1.0)
			scale = 1.0 / scale;
		else
			scale = 1.0;

		while ((readcount = sf_read_float (sndFile, buffer, MUSIC_BUFFER_SIZE)) && 
				!atomic_load_explicit(&isExit, memory_order_relaxed)) {
			float peak = 0;
			float avg = 0;

			for (int m = 0 ; m < readcount ; m++) {
				buffer [m] *= scale ;
				float s = fabs(buffer[m]);
				if (s > peak) peak = s;
				avg += s;
			}
			avg /= readcount;

			// EMA smothing peak
			float prePeak = atomic_load_explicit(&state->peakAmp, memory_order_relaxed);
			if (peak > prePeak)
				prePeak = PEAK_ALPHA_ATTACK  * peak + (1 - PEAK_ALPHA_ATTACK)  * prePeak;
			else 
				prePeak = PEAK_ALPHA_RELEASE * peak + (1 - PEAK_ALPHA_RELEASE) * prePeak;

			// EMA smothing avg
			float preAvg = atomic_load_explicit(&state->avgAmp, memory_order_relaxed);
			preAvg = AVG_ALPHA * avg + (1 - AVG_ALPHA) * preAvg;

			atomic_store_explicit(&state->peakAmp, prePeak, memory_order_relaxed);
			atomic_store_explicit(&state->avgAmp,  preAvg,  memory_order_relaxed);

			alsa_write_float (pcmHandle, buffer, MUSIC_BUFFER_SIZE / info.channels, info.channels) ;

			mtx_lock(&state->pauseMX);
			while (state->isPaused) {
				snd_pcm_pause(pcmHandle, 1);
				cnd_wait(&state->pauseCV, &state->pauseMX);
				if (atomic_load_explicit(&isExit, memory_order_relaxed)) break;
				snd_pcm_pause(pcmHandle, 0);
			}
			mtx_unlock(&state->pauseMX);

			int seek = atomic_load_explicit(&state->seekNow, memory_order_relaxed);
			if (seek) {
				sf_seek(sndFile, seek * info.samplerate, SEEK_CUR);
				atomic_store_explicit(&state->seekNow, 0, memory_order_relaxed);
			}

			atomic_store_explicit(&state->positionSec,
					sf_seek(sndFile, 0, SEEK_CUR) / info.samplerate,
					memory_order_relaxed);
		}
	} else {
		while ((readcount = sf_read_float (sndFile, buffer, MUSIC_BUFFER_SIZE)) && 
				!atomic_load_explicit(&isExit, memory_order_relaxed)) {
			float peak = 0;
			float avg = 0;

			for (int m = 0 ; m < readcount ; m++) {
				float s = fabs(buffer[m]);
				if (s > peak) peak = s;
				avg += s;
			}
			avg /= readcount;

			// EMA smothing peak
			float prePeak = atomic_load_explicit(&state->peakAmp, memory_order_relaxed);
			if (peak > prePeak)
				prePeak = PEAK_ALPHA_ATTACK  * peak + (1 - PEAK_ALPHA_ATTACK)  * prePeak;
			else 
				prePeak = PEAK_ALPHA_RELEASE * peak + (1 - PEAK_ALPHA_RELEASE) * prePeak;

			// EMA smothing avg
			float preAvg = atomic_load_explicit(&state->avgAmp, memory_order_relaxed);
			preAvg = AVG_ALPHA * avg + (1 - AVG_ALPHA) * preAvg;

			atomic_store_explicit(&state->peakAmp, prePeak, memory_order_relaxed);
			atomic_store_explicit(&state->avgAmp,  preAvg,  memory_order_relaxed);

			alsa_write_float (pcmHandle, buffer, MUSIC_BUFFER_SIZE/ info.channels, info.channels);

			mtx_lock(&state->pauseMX);
			while (state->isPaused) {
				snd_pcm_pause(pcmHandle, 1);
				cnd_wait(&state->pauseCV, &state->pauseMX);
				if (atomic_load_explicit(&isExit, memory_order_relaxed)) break;
				snd_pcm_pause(pcmHandle, 0);
			}
			mtx_unlock(&state->pauseMX);

			int seek = atomic_load_explicit(&state->seekNow, memory_order_relaxed);
			if (seek) {
				sf_seek(sndFile, seek * info.samplerate, SEEK_CUR);
				atomic_store_explicit(&state->seekNow, 0, memory_order_relaxed);
			}

			atomic_store_explicit(&state->positionSec, 
					sf_seek(sndFile, 0, SEEK_CUR) / info.samplerate, 
					memory_order_relaxed);
		}
	}

	sf_close(sndFile);
	snd_pcm_drain(pcmHandle);
	snd_pcm_close(pcmHandle);
	atomic_store_explicit(&isExit, true, memory_order_relaxed);
	return 0;
}

static void pauseAudio(State* state)
{
	mtx_lock(&state->pauseMX);
	state->isPaused = true;
	mtx_unlock(&state->pauseMX);
}

static void resumeAudio(State* state) 
{
	mtx_lock(&state->pauseMX);
	state->isPaused = false;
	cnd_signal(&state->pauseCV);
	mtx_unlock(&state->pauseMX);
}

#ifndef NDEBUG
static void glfwErrorCallback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}
#endif

static void glfwFramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	State* state = glfwGetWindowUserPointer(window);

	glViewport(0, 0, width, height);
	if (!state->fullscreen) {
		state->winWidth = width;
		state->winHeight = height;
	}

	glUniform2ui(state->uniformLocResolution, width, height);
}

static void glfwWindowPosCallback(GLFWwindow* window, int xpos, int ypos) 
{
	State* state = glfwGetWindowUserPointer(window);
	if (!state->fullscreen) {
		state->winPosX = xpos;
		state->winPosY = ypos;
	}
}

static void glfwCursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	State* state = glfwGetWindowUserPointer(window);
	glUniform2f(state->uniformLocMouse, xpos, ypos);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	state->isCursorHidden = false;
	state->prevCursorTime = state->currentTime;
}

static void glfwCursorEnterCallback(GLFWwindow* window, int entered) 
{
	State* state = glfwGetWindowUserPointer(window);
	state->isCursorInside = (entered == GLFW_TRUE);
}

static void glfwKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	State* state = glfwGetWindowUserPointer(window);

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
		if (state->fullscreen) {
			glfwSetWindowMonitor(state->window, NULL, state->winPosX, state->winPosY,
					state->winWidth,
					state->winHeight,
					0);
			state->fullscreen = false;
		} else {
			glfwSetWindowMonitor(state->window, primary, 0, 0,
					mode->width,
					mode->height,
					mode->refreshRate);
			state->fullscreen = true;
		}
	}
	else if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		if (state->isPaused)
			resumeAudio(state);
		else
			pauseAudio(state);
	}
	else if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS) {
		atomic_fetch_add_explicit(&state->seekNow, MUSIC_CONTROL_SLOW_SEC, memory_order_relaxed);
	}
	else if (key == GLFW_KEY_LEFT && action == GLFW_PRESS) {
		atomic_fetch_sub_explicit(&state->seekNow, MUSIC_CONTROL_SLOW_SEC, memory_order_relaxed);
	}
	else if (key == GLFW_KEY_UP && action == GLFW_PRESS) {
		atomic_fetch_add_explicit(&state->seekNow, MUSIC_CONTROL_FAST_SEC, memory_order_relaxed);
	}
	else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) {
		atomic_fetch_sub_explicit(&state->seekNow, MUSIC_CONTROL_FAST_SEC, memory_order_relaxed);
	}
	else if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS) {
		atomic_fetch_add_explicit(&state->ampScale, AMP_SCALE_CONTROL_UNIT, memory_order_relaxed);
	}
	else if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_PRESS) {
		atomic_fetch_sub_explicit(&state->ampScale, AMP_SCALE_CONTROL_UNIT, memory_order_relaxed);
	}
}

static void clearLineAnsi()
{
	printf("\033[2K\r");
	fflush(stdout);
}

// Initialize the window for opengl
static bool initWindow(State* state, const int32_t width, const int32_t height)
{
	state->winWidth = width;
	state->winHeight = height;

#ifndef NDEBUG
	glfwSetErrorCallback(glfwErrorCallback);
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
		xpos = (mode->width - width) / 2;
		ypos = (mode->height - height) / 2;
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);
        } else {
		state->fullscreen = false;
	}

	if (state->fullscreen) {
		state->window = glfwCreateWindow(mode->width, mode->height, "FreeVisualizer", primary, NULL);
	} else {
		state->window = glfwCreateWindow(width, height, "FreeVisualizer", NULL, NULL);
	}

	if (!state->window)
		return false;

	// Set the internal WindowPointer for glfw to access states in callback functions
	glfwSetWindowUserPointer(state->window, state);

	if (mode)
		glfwSetWindowPos(state->window, xpos, ypos);

	glfwMakeContextCurrent(state->window);
	glfwSetFramebufferSizeCallback(state->window, glfwFramebufferSizeCallback);
	glfwSetWindowPosCallback(state->window, glfwWindowPosCallback);
	glfwSetKeyCallback(state->window, glfwKeyCallback);
	glfwSetCursorPosCallback(state->window, glfwCursorPosCallback);
	glfwSetCursorEnterCallback(state->window, glfwCursorEnterCallback);

	// setting v-sync
	glfwSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		fprintf(stderr, "Failed to initialize GLAD!\n");
		return false;
	}

	return true;
}

// Load and compile glsl shaders
static bool initShaders(State* state, const char* fragShaderPath)
{
	// From: https://stackoverflow.com/a/59739538 this beautiful answer by derhass
	const char *vertexShaderSource = "#version 330 core\n"
		"void main()\n"
		"{\n"
		"vec2 vertices[3]=vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1, 3));\n"
		"gl_Position = vec4(vertices[gl_VertexID],0,1);\n"
		"}\0";

	if (!createShaderFromSrc(vertexShaderSource, &state->vertShader, GL_VERTEX_SHADER))
		return false;

	if (!createShaderFromPath(fragShaderPath, &state->fragShader, GL_FRAGMENT_SHADER)) {
		glDeleteShader(state->vertShader);
		return false;
	}

	if (!createProgrm(state->vertShader, state->fragShader, &state->shaderProgram)) {
		glDeleteShader(state->vertShader);
		glDeleteShader(state->fragShader);
		return false;
	}

	glDeleteShader(state->vertShader);
	glDeleteShader(state->fragShader);

	glUseProgram(state->shaderProgram);

	// Retriving uniform locations
	state->uniformLocResolution = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_RESOLUTION);
	state->uniformLocMouse = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_MOUSE);
	state->uniformLocTime = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_TIME);
	state->uniformLocPeakAmp = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_PEAKAMP);
	state->uniformLocAvgAmp = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_AVGAMP);
	state->uniformLocColor1 = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_COLOR1);
	state->uniformLocColor2 = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_COLOR2);
	state->uniformLocColor3 = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_COLOR3);
	state->uniformLocColor4 = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_COLOR4);

	// Uploading colors (as they are const)
	glUniform3f(state->uniformLocColor1, state->colors[0][0], state->colors[0][1], state->colors[0][2]);
	glUniform3f(state->uniformLocColor2, state->colors[1][0], state->colors[1][1], state->colors[1][2]);
	glUniform3f(state->uniformLocColor3, state->colors[2][0], state->colors[2][1], state->colors[2][2]);
	glUniform3f(state->uniformLocColor4, state->colors[3][0], state->colors[3][1], state->colors[3][2]);
	

	// First time glViewport and Resolution uniform upload
	glfwFramebufferSizeCallback(state->window, state->winWidth, state->winHeight);

	return true;
}

// Loading the music and init audio system (alsa)
static bool initAudio(State* state)
{
	SF_INFO info = { 0 };
	SNDFILE* sndFile = sf_open(state->musicPath, SFM_READ, &info);

	if (!sndFile) {
		fprintf(stderr, "Error opening file: %s\n", sf_strerror(NULL));
		return false;
	}

	state->duration = info.frames / info.samplerate;
	sf_close(sndFile);

	if (mtx_init(&state->pauseMX, mtx_plain) != thrd_success) {
		fprintf(stderr, "Error creating mutex for music thread.\n");
		return false;
	}

	if (cnd_init(&state->pauseCV) != thrd_success) {
		fprintf(stderr, "Error creating conditional variable for music thread.\n");
		return false;
	}

	if (thrd_create(&state->musicThread, audio_playback_callback, state) != thrd_success) {
		fprintf(stderr, "Error creating music thread.\n");
		return false;
	}

	return true;
}

// Main drawing loop
static void loop(State* state)
{
	GLuint dummyVAO; 
	glGenVertexArrays(1, &dummyVAO);
	glBindVertexArray(dummyVAO);

	while (!glfwWindowShouldClose(state->window) && !atomic_load_explicit(&isExit, memory_order_relaxed))
	{
		// Updating current time
		state->currentTime = glfwGetTime();

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Printing duration and position
		clearLineAnsi();
		fprintf(stdout, "[%s] %d/%d seconds",
				(state->isPaused) ? "Paused" : "Playing",
				atomic_load_explicit(&state->positionSec, memory_order_relaxed),
				state->duration);
		fflush(stdout);

		// Uploading uniforms
		glUniform1f(state->uniformLocTime, state->currentTime);
		glUniform1f(state->uniformLocPeakAmp, 
				atomic_load_explicit(&state->peakAmp, memory_order_relaxed) *
				atomic_load_explicit(&state->ampScale, memory_order_relaxed) * 0.1);
		glUniform1f(state->uniformLocAvgAmp, 
				atomic_load_explicit(&state->avgAmp, memory_order_relaxed) *
				atomic_load_explicit(&state->ampScale, memory_order_relaxed) * 0.1);

		// Drawing
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwSwapBuffers(state->window);
		glfwPollEvents();

		if (!state->isCursorHidden && 
		    (state->currentTime - state->prevCursorTime) > CURSOR_INACTIVE_LIMIT &&
		     state->isCursorInside) {
			state->isCursorHidden = true;
			glfwSetInputMode(state->window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		}
	}
}

// Release all resources
static void deinitApp(State* state)
{
	// Killing playback thread
	if (!state->testMode) {
		atomic_store_explicit(&isExit, true, memory_order_relaxed);
		mtx_lock(&state->pauseMX);
		cnd_broadcast(&state->pauseCV);
		mtx_unlock(&state->pauseMX);
		thrd_join(state->musicThread, NULL);
	}

	if (state->renderSub) {
		free_srt(state->srtHandle);
	}

	glDeleteProgram(state->shaderProgram);
	glfwDestroyWindow(state->window);
	glfwTerminate();
	// Show terminal cursor (if hidden)
	printf("\033[?25h");
	fflush(stdout);
}

// Callback function for Ctrl+C
static void catchCtrlC(int sig)
{
	atomic_store_explicit(&isExit, true, memory_order_relaxed);
}
