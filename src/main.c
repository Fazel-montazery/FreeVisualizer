#ifdef __STDC_NO_THREADS__
#error "<threads.h> not supported"
#endif

#include <threads.h>
#include <stdatomic.h>

#include <signal.h>
#include <math.h>

#include "../include/glad/glad.h"
#include <GLFW/glfw3.h>

#include <pipewire/pipewire.h>

#include <sndfile.h>

#include "defs.h"
#include "opts.h"
#include "fs.h"
#include "shader.h"
#include "state.h"

// Blueprints
static bool initWindow(State* state, const int32_t width, const int32_t height);
static bool initShaders(State* state, const char* fragShaderPath);
static bool initAudio(State* state);
static void loop(State* state);
static void deinitApp(State* state);
static void catchCtrlC(int sig);
static void readSavedColors(State* state);
static void writeSavedColors(State* state);

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

	// Setting up the Ctrl+C and kill signals
	signal(SIGINT, catchCtrlC);
	signal(SIGTERM, catchCtrlC);

	if (!parseOpts(argc, argv, &state))
		return 0;

	readSavedColors(&state);

	if (!initWindow(&state, DEFAULT_WIN_WIDTH, DEFAULT_WIN_HEIGHT)) return -1;
	if (!initShaders(&state, state.fragShaderPath)) return -1;
	if (!state.testMode)
		if (!initAudio(&state)) return -1;
	loop(&state);
	deinitApp(&state);
	return 0;
}

// Helper functions
static void uploadColorsUniform(State* state)
{
	glUniform3f(state->uniformLocColor1, state->colors[0][0], state->colors[0][1], state->colors[0][2]);
	glUniform3f(state->uniformLocColor2, state->colors[1][0], state->colors[1][1], state->colors[1][2]);
	glUniform3f(state->uniformLocColor3, state->colors[2][0], state->colors[2][1], state->colors[2][2]);
	glUniform3f(state->uniformLocColor4, state->colors[3][0], state->colors[3][1], state->colors[3][2]);
}

static void readSavedColors(State* state)
{
	for (int i = 0; i < NUM_SAVED_COLORS; i++)
		copyDefaultColors(state->savedColors[i]);

	char* colors = loadFileToStr(state->savedColorsPath);
	if (!colors) {
		fprintf(stdout, "Continuing without any previous fav colors...\n");
	} else {
		char* token = strtok(colors, " \t\n\v\f\r");
		int i = 0;
		while (token && (i < NUM_SAVED_COLORS)) {
			for (int j = 0; (j < NUM_COLORS) && token; j++) {
				for (int k = 0; (k < 3) && token; k++) {
					state->savedColors[i][j][k] = atof(token);
					token = strtok(NULL, " \t\n\v\f\r");
				}
			}
			i++;
		}
		free(colors);
	}
}

static void writeSavedColors(State* state)
{
	FILE* f = fopen(state->savedColorsPath, "w");
	if (!f) {
		fprintf(stdout, "Couldn't open file [%s] => %s\n", state->savedColorsPath, strerror(errno));
		fprintf(stdout, "Couldn't Save fav colors!\n");
	} else {
		for (int i = 0; i < NUM_SAVED_COLORS; i++) {
			for (int j = 0; j < NUM_COLORS; j++) {
				fprintf(f, "%f %f %f ", state->savedColors[i][j][0],
							state->savedColors[i][j][1],
							state->savedColors[i][j][2]);
			}
			fputc('\n', f);
		}
		fclose(f);
	}
}

static void refillRingBuffer(State* state)
{
        int32_t filled;
        uint32_t index, avail;
        uint64_t count;
	uint32_t n_frames = RING_BUFFER_SIZE;

	while (n_frames > 0) {
		bool flag = true;
                while (flag) {
                        filled = spa_ringbuffer_get_write_index(&state->spaRing, &index);
 
                        /* this is how much samples we can write */
                        avail = RING_BUFFER_SIZE - filled;
                        if (avail > 0)
                                break;
 
                        /* no space.. block and wait for free space */
                        spa_system_eventfd_read(state->pwLoop->system, state->spaEventFd, &count);
			if (atomic_load_explicit(&isExit, memory_order_relaxed))
				flag = false;
                }
		if (!flag) break;

                if (avail > n_frames)
                        avail = n_frames;
 
		/* write new samples to the ringbuffer from the given index */
		uint32_t offset = index % RING_BUFFER_SIZE;
		uint32_t space_to_end = RING_BUFFER_SIZE - offset;
		uint32_t readCount1 = sf_readf_float(state->sndfile, &state->ringBuffer[offset * state->sndinfo.channels], SPA_MIN(avail, space_to_end));
		uint32_t readCount2 = 0;
		if (readCount1 < avail) {
			readCount2 = sf_readf_float(state->sndfile, &state->ringBuffer[0], avail - readCount1);
		}

                n_frames -= readCount1 + readCount2;
 
                /* and advance the ringbuffer */
                spa_ringbuffer_write_update(&state->spaRing, index + readCount1 + readCount2);
        }
}

static int audioProducerCallback(void* userdata)
{
	while (!atomic_load_explicit(&isExit, memory_order_relaxed)) {
		refillRingBuffer((State*)userdata);
	}
	return 0;
}

static void pwOnProcess(void *userdata)
{
	State* state = userdata;
        struct pw_buffer *b;
        struct spa_buffer *buf;
        uint8_t *p;
        uint32_t index, to_read, to_silence;
        int32_t avail, n_frames, stride;

        if ((b = pw_stream_dequeue_buffer(state->pwStream)) == NULL) {
                pw_log_warn("out of buffers: %m");
                return;
        }

        buf = b->buffer;
        if ((p = buf->datas[0].data) == NULL)
                return;

        /* the amount of space in the ringbuffer and the read index */
        avail = spa_ringbuffer_get_read_index(&state->spaRing, &index);

        stride = sizeof(float) * state->sndinfo.channels;
        n_frames = buf->datas[0].maxsize / stride;
        if (b->requested)
                n_frames = SPA_MIN((int32_t)b->requested, n_frames);

        /* we can read if there is something available */
        to_read = avail > 0 ? SPA_MIN(avail, n_frames) : 0;
        /* and fill the remainder with silence */
        to_silence = n_frames - to_read;

        if (to_read > 0) {
                /* read data into the buffer */
                spa_ringbuffer_read_data(&state->spaRing,
                                state->ringBuffer, RING_BUFFER_SIZE * stride,
                                (index % RING_BUFFER_SIZE) * stride,
                                p, to_read * stride);
                /* update the read pointer */
                spa_ringbuffer_read_update(&state->spaRing, index + to_read);
        }
        if (to_silence > 0)
                /* set the rest of the buffer to silence */
                memset(SPA_PTROFF(p, to_read * stride, void), 0, to_silence * stride);

        buf->datas[0].chunk->offset = 0;
        buf->datas[0].chunk->stride = stride;
        buf->datas[0].chunk->size = n_frames * stride;

        pw_stream_queue_buffer(state->pwStream, b);

        /* signal the main thread to fill the ringbuffer */
        spa_system_eventfd_write(state->pwLoop->system, state->spaEventFd, 1);
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

	if (action == GLFW_PRESS) {
	if (key == GLFW_KEY_ESCAPE) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	else if (key == GLFW_KEY_Q) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	else if (key == GLFW_KEY_F) {
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
	else if (key == GLFW_KEY_SPACE) {
		if (state->isPaused)
			resumeAudio(state);
		else
			pauseAudio(state);
	}
	else if (key == GLFW_KEY_RIGHT) {
		atomic_fetch_add_explicit(&state->seekNow, MUSIC_CONTROL_SLOW_SEC, memory_order_relaxed);
	}
	else if (key == GLFW_KEY_LEFT) {
		atomic_fetch_sub_explicit(&state->seekNow, MUSIC_CONTROL_SLOW_SEC, memory_order_relaxed);
	}
	else if (key == GLFW_KEY_UP) {
		atomic_fetch_add_explicit(&state->seekNow, MUSIC_CONTROL_FAST_SEC, memory_order_relaxed);
	}
	else if (key == GLFW_KEY_DOWN) {
		atomic_fetch_sub_explicit(&state->seekNow, MUSIC_CONTROL_FAST_SEC, memory_order_relaxed);
	}
	else if (key == GLFW_KEY_RIGHT_BRACKET) {
		atomic_fetch_add_explicit(&state->ampScale, AMP_SCALE_CONTROL_UNIT, memory_order_relaxed);
	}
	else if (key == GLFW_KEY_LEFT_BRACKET) {
		atomic_fetch_sub_explicit(&state->ampScale, AMP_SCALE_CONTROL_UNIT, memory_order_relaxed);
	}
	else if (key == GLFW_KEY_R) {
		randColors(state);
		uploadColorsUniform(state);
	}
	else if (key == GLFW_KEY_S) {
		atomic_store_explicit(&state->seekNow, MUSIC_CONTROL_SEEK_START, memory_order_relaxed);
	}
	else if (FVGLFW_NUMS_KEY_CONDITION(key)) {
		int num = FVGLFW_NUMS_KEY_INT(key);
		if (mods & GLFW_MOD_SHIFT) {
			for (int i = 0; i < NUM_COLORS; i++) {
				state->savedColors[num][i][0] = state->colors[i][0];
				state->savedColors[num][i][1] = state->colors[i][1];
				state->savedColors[num][i][2] = state->colors[i][2];
			}
		} else {
			for (int i = 0; i < NUM_COLORS; i++) {
				state->colors[i][0] = state->savedColors[num][i][0];
				state->colors[i][1] = state->savedColors[num][i][1];
				state->colors[i][2] = state->savedColors[num][i][2];
			}
			uploadColorsUniform(state);
		}
	}
	} // if (action == GLFW_PRESS)
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
	uploadColorsUniform(state);

	// First time glViewport and Resolution uniform upload
	glfwFramebufferSizeCallback(state->window, state->winWidth, state->winHeight);

	return true;
}

// Loading the music and init audio system
static bool initAudio(State* state)
{
	state->sndfile = sf_open(state->musicPath, SFM_READ, &state->sndinfo);

	if (!state->sndfile) {
		fprintf(stderr, "Error opening file: %s\n", sf_strerror(NULL));
		goto audio_deinit;
	}

	state->ringBuffer = malloc(RING_BUFFER_SIZE * state->sndinfo.channels * sizeof(float));
	if (!state->ringBuffer) {
		fprintf(stderr, "Couldn't allocate the RingBuffer: %s\n", strerror(errno));
		goto audio_deinit_0;
	}

	state->duration = state->sndinfo.frames / state->sndinfo.samplerate;

	// Pipewire
	state->spaPodBuilder = SPA_POD_BUILDER_INIT(state->spaPodbuffer, 1024);
	pw_init(NULL, NULL);

	state->pwThreadLoop = pw_thread_loop_new("FreeVisualizerAudio", NULL);
	if (!state->pwThreadLoop) {
		fprintf(stderr, "Couldn't create pipewire's ThreadLoop: %s\n", strerror(errno));
		goto audio_deinit_1;
	}
	state->pwLoop = pw_thread_loop_get_loop(state->pwThreadLoop);

	pw_thread_loop_lock(state->pwThreadLoop);

	if ((state->spaEventFd = spa_system_eventfd_create(state->pwLoop->system, SPA_FD_CLOEXEC)) < 0) {
		fprintf(stderr, "Couldn't create spa eventfd: %s\n", strerror(errno));
		goto audio_deinit_2;
	}

	if(pw_thread_loop_start(state->pwThreadLoop) != 0) {
		fprintf(stderr, "Couldn't start pipewire ThreadLoop: %s\n", strerror(errno));
		goto audio_deinit_3;
	}

	spa_ringbuffer_init(&state->spaRing);

	// Setup Stream
	state->pwProps = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio",
			 PW_KEY_MEDIA_CATEGORY, "Playback",
			 PW_KEY_MEDIA_ROLE, "Music",
			 NULL);

	if (!state->pwProps) {
		fprintf(stderr, "Couldn't create pipewire's stream properties: %s\n", strerror(errno));
		goto audio_deinit_3;
	}

	static const struct pw_stream_events streamEvents = {
		PW_VERSION_STREAM_EVENTS,
		.process = pwOnProcess,
	};

	state->pwStream = pw_stream_new_simple(state->pwLoop,
					       "FreeVisualizer",
					       state->pwProps,
					       &streamEvents,
					       state);

	if (!state->pwStream) {
		fprintf(stderr, "Couldn't create pipewire's stream object: %s\n", strerror(errno));
		goto audio_deinit_3;
	}

	state->spaParams[0] = spa_format_audio_raw_build(&state->spaPodBuilder, SPA_PARAM_EnumFormat,
							 &SPA_AUDIO_INFO_RAW_INIT(
								 .format = SPA_AUDIO_FORMAT_F32,
								 .channels = state->sndinfo.channels,
								 .rate = state->sndinfo.samplerate));

	if (!state->spaParams[0]) {
		fprintf(stderr, "Couldn't create spa pod: %s\n", strerror(errno));
		goto audio_deinit_4;
	}

	int res = pw_stream_connect(state->pwStream,
		  PW_DIRECTION_OUTPUT,
		  PW_ID_ANY,
		  PW_STREAM_FLAG_AUTOCONNECT |
		  PW_STREAM_FLAG_MAP_BUFFERS |
		  PW_STREAM_FLAG_RT_PROCESS,
		  state->spaParams, 1);

	if (res < 0) {
		fprintf(stderr, "Couldn't connect to pipewire stream: %s\n", strerror(errno));
		goto audio_deinit_4;
	}

	// Prefill the ringbuffer
	refillRingBuffer(state);

	// Start Audio Producer
	if (thrd_create(&state->audioProducerThread, audioProducerCallback, state) != thrd_success) {
		fprintf(stderr, "Couldn't create AudioProducer thread: %s\n", strerror(errno));
		goto audio_deinit_4;
	}

        pw_thread_loop_unlock(state->pwThreadLoop);

	if (mtx_init(&state->pauseMX, mtx_plain) != thrd_success) {
		fprintf(stderr, "Error creating mutex for music thread.\n");
		free(state->ringBuffer);
		return false;
	}

	if (cnd_init(&state->pauseCV) != thrd_success) {
		fprintf(stderr, "Error creating conditional variable for music thread.\n");
		free(state->ringBuffer);
		return false;
	}

	return true;

audio_deinit_4:
	pw_stream_destroy(state->pwStream);
audio_deinit_3:
	close(state->spaEventFd);
audio_deinit_2:
	pw_thread_loop_unlock(state->pwThreadLoop);
	pw_thread_loop_destroy(state->pwThreadLoop);
audio_deinit_1:
	pw_deinit();
	free(state->ringBuffer);
audio_deinit_0:
	sf_close(state->sndfile);
audio_deinit:
	return false;
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

		const int currentSecs = atomic_load_explicit(&state->positionSec, memory_order_relaxed);

		if (state->renderSub) {
			SrtTimeStamp currentTs = convertSecsToSrtTs(currentSecs);
			SrtSection* currentSection = getSectionByTime(&state->srtHandle, currentTs);
		}

		fprintf(stdout, "[%s] %d/%d seconds",
				(state->isPaused) ? "Paused" : "Playing",
				currentSecs,
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
	atomic_store_explicit(&isExit, true, memory_order_relaxed);

	// Save fav colors
	writeSavedColors(state);

	// Join producer thread
	spa_system_eventfd_write(state->pwLoop->system, state->spaEventFd, 1);
	thrd_join(state->audioProducerThread, NULL);

	// Deinit pipwire
	pw_thread_loop_lock(state->pwThreadLoop);
        pw_stream_destroy(state->pwStream);
	pw_thread_loop_unlock(state->pwThreadLoop);
	pw_thread_loop_destroy(state->pwThreadLoop);
	close(state->spaEventFd);
	pw_deinit();

	// Free RingBuffer
	free(state->ringBuffer);

	// sndfile
	sf_close(state->sndfile);

	// Killing playback thread
	if (!state->testMode) {
		atomic_store_explicit(&isExit, true, memory_order_relaxed);
		mtx_lock(&state->pauseMX);
		cnd_broadcast(&state->pauseCV);
		mtx_unlock(&state->pauseMX);
	}

	free_srt(state->srtHandle);

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
