#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <math.h>

#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>

#include <alsa/asoundlib.h>
#include <sndfile.h>

#include "defs.h"
#include "opts.h"
#include "shader.h"

// App State
typedef struct
{
	// Window
	GLFWwindow* window;
	int32_t winWidth; // not in fullscreen mode
	int32_t winHeight; // not in fullscreen mode
	int32_t winPosX;
	int32_t winPosY;
	bool fullscreen;

	// Music
	SNDFILE   *sndFile;
	snd_pcm_t *pcmHandle;
	int16_t* musicBuffer;
	pthread_t musicThread;
	int channels;

	// Shaders
	GLuint vertShader;
	GLuint fragShader;
	GLuint shaderProgram;
	GLint uniformLocResolution;
	GLint uniformLocMouse;
	GLint uniformLocTime;
	GLint uniformLocPeakAmp;
	GLint uniformLocAvgAmp;

} State;

// Blueprints
static bool initWindow(State* state, const int32_t width, const int32_t height);
static bool initShaders(State* state, const char* fragShaderPath);
static bool initAudio(State* state, const char* musicPath);
static void loop(const State state);
static void deinitApp(State* state);
static void catchCtrlC(int sig);

// Globs
static bool isExit = false;
static float peakAmp = 0.0;
static float avgAmp = 0.0;

// Main
int main(int argc, char** argv)
{
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
}

static void* audio_playback_callback(void* arg)
{
	State* state = arg;

	sf_count_t frames_read;
	while ((frames_read = sf_readf_short(state->sndFile, state->musicBuffer, MUSIC_BUFFER_SIZE)) > 0) {
		if (isExit) return NULL;

		int16_t* samples = state->musicBuffer;
		int numSamples = frames_read * state->channels;

		int16_t peak = 0;
		int32_t sum = 0;
		for (int i = 0; i < numSamples; i++) {
			int16_t v = samples[i];
			int16_t abs_v = v < 0 ? -v : v;
			if (abs_v > peak) peak = abs_v;
			sum += abs_v;
		}

		peakAmp = (float)peak / INT16_MAX;
		avgAmp = (float)sum / numSamples / INT16_MAX;

		snd_pcm_sframes_t frames_written = 
			snd_pcm_writei(state->pcmHandle, state->musicBuffer, frames_read);

		if (frames_written < 0) {
			frames_written = snd_pcm_recover(state->pcmHandle, frames_written, 0);
		}

		if (frames_written < 0) {
			fprintf(stderr, "ALSA write error: %s\n", snd_strerror(frames_written));
			break;
		}
	}

	return NULL;
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
static bool initAudio(State* state, const char* musicPath)
{
	SF_INFO info;
	state->sndFile = sf_open(musicPath, SFM_READ, &info);

	if (!state->sndFile) {
		fprintf(stderr, "Error opening file: %s\n", sf_strerror(NULL));
		return false;
	}

	unsigned int rate    = info.samplerate;
	unsigned int chans   = info.channels;
	state->channels = info.channels;

	int err;
	if ((err = snd_pcm_open(&state->pcmHandle, PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		fprintf(stderr, "ALSA device open error: %s\n", snd_strerror(err));
		return false;
	}

	err = snd_pcm_set_params(state->pcmHandle,
                             SND_PCM_FORMAT_S16_LE,
                             SND_PCM_ACCESS_RW_INTERLEAVED,
                             chans,
                             rate,
                             1,
                             100000); // 0.1 sec latency

	if (err < 0) {
		fprintf(stderr, "ALSA set params error: %s\n", snd_strerror(err));
		return false;
	}

	state->musicBuffer = malloc(MUSIC_BUFFER_SIZE * chans * sizeof(int16_t));
	if (!state->musicBuffer) {
		fprintf(stderr, "Music buffer allocation failed!\n");
		return false;
	}

	pthread_create(&state->musicThread, NULL, audio_playback_callback, state);

	return true;
}

// Main drawing loop
static void loop(const State state)
{
	GLuint dummyVAO; 
	glGenVertexArrays(1, &dummyVAO);
	glBindVertexArray(dummyVAO);

	while (!glfwWindowShouldClose(state.window) && !isExit)
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
		glUniform1f(state.uniformLocPeakAmp, peakAmp);
		glUniform1f(state.uniformLocAvgAmp, avgAmp);

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
	isExit = true;
	pthread_join(state->musicThread, NULL);

	glDeleteProgram(state->shaderProgram);
	glfwDestroyWindow(state->window);
	glfwTerminate();
	if (state->musicBuffer) free(state->musicBuffer);
	sf_close(state->sndFile);
	snd_pcm_drain(state->pcmHandle);
	snd_pcm_close(state->pcmHandle);
	// Show terminal cursor (if hidden)
	printf("\033[?25h");
	fflush(stdout);
	exit(0);
}

// Callback function for Ctrl+C
static void catchCtrlC(int sig)
{
	isExit = true;
}
