#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <stdatomic.h>
#include <math.h>

#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>

#include "defs.h"
#include "opts.h"
#include "shader.h"
#include "sound.h"

// App State
typedef struct
{
	// Window
	GLFWwindow*	window;
	int32_t		winWidth; // not in fullscreen mode
	int32_t		winHeight; // not in fullscreen mode
	int32_t		winPosX;
	int32_t		winPosY;
	bool		fullscreen;

	// Music
	pthread_t	musicThread;

	// Shaders
	GLuint		vertShader;
	GLuint		fragShader;
	GLuint		shaderProgram;
	GLint		uniformLocResolution;
	GLint		uniformLocMouse;
	GLint		uniformLocTime;
	GLint		uniformLocPeakAmp;
	GLint		uniformLocAvgAmp;

} State;

// Blueprints
static bool initWindow(State* state, const int32_t width, const int32_t height);
static bool initShaders(State* state, const char* fragShaderPath);
static bool initAudio(State* state, char* musicPath);
static void loop(const State state);
static void deinitApp(State* state);
static void catchCtrlC(int sig);

// Globs
static atomic_bool isExit = false;

static bool isPaused = false;
static pthread_mutex_t pauseMX = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pauseCV = PTHREAD_COND_INITIALIZER;
static _Atomic float peakAmp = 0.0;
static _Atomic float avgAmp = 0.0;

// Main
int main(int argc, char** argv)
{
	// Initializing atomics
	atomic_store_explicit(&isExit, false, memory_order_relaxed);
	atomic_store_explicit(&peakAmp, 0.0, memory_order_relaxed);
	atomic_store_explicit(&avgAmp, 0.0, memory_order_relaxed);

	// Initializing the app state
	State state = { 0 };

	// Setting up the Ctrl+C signal
	signal(SIGINT, catchCtrlC);

	char fragShaderPath[PATH_SIZE];
	char* musicPath;
	if (!parseOpts(argc, argv, &musicPath, fragShaderPath, PATH_SIZE, &state.fullscreen))
		return 0;

	if (!initWindow(&state, DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT)) return -1;
	if (!initShaders(&state, fragShaderPath)) return -1;
	if (!initAudio(&state, musicPath)) return -1;
	loop(state);
	deinitApp(&state);
	return 0;
}

// Helper functions
static void* audio_playback_callback(void* arg)
{
	char* musicPath = arg;

	static float buffer [MUSIC_BUFFER_SIZE];

	SF_INFO info = { 0 };
	SNDFILE* sndFile = sf_open(musicPath, SFM_READ, &info);

	if (!sndFile) {
		fprintf(stderr, "Error opening file: %s\n", sf_strerror(NULL));
		atomic_store_explicit(&isExit, true, memory_order_relaxed);
		return NULL;
	}

	if (info.channels < 1 || info.channels > 2) {	
		fprintf (stderr, "Error : channels = %d\n", info.channels);
		atomic_store_explicit(&isExit, true, memory_order_relaxed);
		return NULL;
	}

	snd_pcm_t* pcmHandle = alsa_open(info.channels, (unsigned) info.samplerate, SF_TRUE);
	if (!pcmHandle) {
		atomic_store_explicit(&isExit, true, memory_order_relaxed);
		return NULL;
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
			float prePeak = atomic_load_explicit(&peakAmp, memory_order_relaxed);
			if (peak > prePeak)
				prePeak = PEAK_ALPHA_ATTACK  * peak + (1 - PEAK_ALPHA_ATTACK)  * prePeak;
			else 
				prePeak = PEAK_ALPHA_RELEASE * peak + (1 - PEAK_ALPHA_RELEASE) * prePeak;

			// EMA smothing avg
			float preAvg = atomic_load_explicit(&avgAmp, memory_order_relaxed);
			preAvg = AVG_ALPHA * avg + (1 - AVG_ALPHA) * preAvg;

			atomic_store_explicit(&peakAmp, prePeak, memory_order_relaxed);
			atomic_store_explicit(&avgAmp,  preAvg,  memory_order_relaxed);

			alsa_write_float (pcmHandle, buffer, MUSIC_BUFFER_SIZE / info.channels, info.channels) ;

			pthread_mutex_lock(&pauseMX);
			while (isPaused) {
				snd_pcm_pause(pcmHandle, 1);
				pthread_cond_wait(&pauseCV, &pauseMX);
				if (atomic_load_explicit(&isExit, memory_order_relaxed)) break;
				snd_pcm_pause(pcmHandle, 0);
			}
			pthread_mutex_unlock(&pauseMX);
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
			float prePeak = atomic_load_explicit(&peakAmp, memory_order_relaxed);
			if (peak > prePeak)
				prePeak = PEAK_ALPHA_ATTACK  * peak + (1 - PEAK_ALPHA_ATTACK)  * prePeak;
			else 
				prePeak = PEAK_ALPHA_RELEASE * peak + (1 - PEAK_ALPHA_RELEASE) * prePeak;

			// EMA smothing avg
			float preAvg = atomic_load_explicit(&avgAmp, memory_order_relaxed);
			preAvg = AVG_ALPHA * avg + (1 - AVG_ALPHA) * preAvg;

			atomic_store_explicit(&peakAmp, prePeak, memory_order_relaxed);
			atomic_store_explicit(&avgAmp,  preAvg,  memory_order_relaxed);

			alsa_write_float (pcmHandle, buffer, MUSIC_BUFFER_SIZE/ info.channels, info.channels);

			pthread_mutex_lock(&pauseMX);
			while (isPaused) {
				snd_pcm_pause(pcmHandle, 1);
				pthread_cond_wait(&pauseCV, &pauseMX);
				if (atomic_load_explicit(&isExit, memory_order_relaxed)) break;
				snd_pcm_pause(pcmHandle, 0);
			}
			pthread_mutex_unlock(&pauseMX);
		}
	}

	sf_close(sndFile);
	snd_pcm_drain(pcmHandle);
	snd_pcm_close(pcmHandle);
	atomic_store_explicit(&isExit, true, memory_order_relaxed);
	return NULL;
}

static void pauseAudio() 
{
	pthread_mutex_lock(&pauseMX);
	isPaused = true;
	pthread_mutex_unlock(&pauseMX);
}

static void resumeAudio() 
{
	pthread_mutex_lock(&pauseMX);
	isPaused = false;
	pthread_cond_signal(&pauseCV);
	pthread_mutex_unlock(&pauseMX);
}

#ifndef NDEBUG
static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}
#endif

static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	State* state = glfwGetWindowUserPointer(window);

	glViewport(0, 0, width, height);
	if (!state->fullscreen) {
		state->winWidth = width;
		state->winHeight = height;
	}
}

static void glfwWindowPosCallback(GLFWwindow* window, int xpos, int ypos) 
{
	State* state = glfwGetWindowUserPointer(window);
	if (!state->fullscreen) {
		state->winPosX = xpos;
		state->winPosY = ypos;
	}
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
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
		if (isPaused)
			resumeAudio();
		else
			pauseAudio();
	}
}

// Initialize the window for opengl
static bool initWindow(State* state, const int32_t width, const int32_t height)
{
	state->winWidth = width;
	state->winHeight = height;

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
	glfwSetFramebufferSizeCallback(state->window, glfw_framebuffer_size_callback);
	glfwSetWindowPosCallback(state->window, glfwWindowPosCallback);
	glfwSetKeyCallback(state->window, glfw_key_callback);

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

	// Retriving uniform locations
	state->uniformLocResolution = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_RESOLUTION);
	state->uniformLocMouse = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_MOUSE);
	state->uniformLocTime = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_TIME);
	state->uniformLocPeakAmp = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_PEAKAMP);
	state->uniformLocAvgAmp = glGetUniformLocation(state->shaderProgram, UNIFORM_NAME_AVGAMP);

	return true;
}

// Loading the music and init audio system (alsa)
static bool initAudio(State* state, char* musicPath)
{
	if (pthread_create(&state->musicThread, NULL, audio_playback_callback, musicPath) != 0) {
		fprintf(stderr, "Error creating music thread.\n");
		return false;
	}
	return true;
}

// Main drawing loop
static void loop(const State state)
{
	GLuint dummyVAO; 
	glGenVertexArrays(1, &dummyVAO);
	glBindVertexArray(dummyVAO);

	while (!glfwWindowShouldClose(state.window) && !atomic_load_explicit(&isExit, memory_order_relaxed))
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Retriving Resolution and mouse pos
		int fbWidth, fbHeight;
		glfwGetFramebufferSize(state.window, &fbWidth, &fbHeight);

		double mouseXpos, mouseYpos;
		glfwGetCursorPos(state.window, &mouseXpos, &mouseYpos);

		// Uploading uniforms
		glUniform2ui(state.uniformLocResolution, fbWidth, fbHeight);
		glUniform2f(state.uniformLocMouse, mouseXpos, mouseYpos);
		glUniform1f(state.uniformLocTime, glfwGetTime());
		glUniform1f(state.uniformLocPeakAmp, atomic_load_explicit(&peakAmp, memory_order_relaxed));
		glUniform1f(state.uniformLocAvgAmp, atomic_load_explicit(&avgAmp, memory_order_relaxed));

		// Drawing
		glUseProgram(state.shaderProgram);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwSwapBuffers(state.window);
		glfwPollEvents();
	}
}

// Release all resources
static void deinitApp(State* state)
{
	// Killing playback thread
	atomic_store_explicit(&isExit, true, memory_order_relaxed);
	pthread_mutex_lock(&pauseMX);
	pthread_cond_broadcast(&pauseCV);
	pthread_mutex_unlock(&pauseMX);
	pthread_join(state->musicThread, NULL);

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
