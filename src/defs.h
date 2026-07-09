#pragma once

#define PCM_DEVICE "default"
#define MUSIC_BUFFER_SIZE (2048)
#define CURSOR_INACTIVE_LIMIT 3

#define PEAK_ALPHA_ATTACK (0.9f)
#define PEAK_ALPHA_RELEASE (0.995f)
#define AVG_ALPHA (0.2f)

#define RING_BUFFER_SIZE (16*1024)
#define RING_BUFFER_WATERMARK_LOW  (RING_BUFFER_SIZE / 8)
#define RING_BUFFER_WATERMARK_HIGH (RING_BUFFER_SIZE * 3/4)
#define MUSIC_CONTROL_SLOW_SEC 2
#define MUSIC_CONTROL_FAST_SEC 5
#define MUSIC_CONTROL_SEEK_START INT8_MIN
#define AMP_SCALE_CONTROL_UNIT 1 // scales back to 0.1 when converted to float
#define AUDIO_MONITOR_SAMPLE_RATE 48000
#define AUDIO_MONITOR_CHANNELS    2

#define PATH_SIZE (1024)
#define DATA_DIR ".FreeVisualizer"
#define SHADER_DIR "shaders"
#define SHADER_EXT ".glsl"
#define SAVED_COLORS_FILE_NAME "colors.txt"
#define NUM_SAVED_COLORS 10
#define FVGLFW_NUMS_KEY_CONDITION(key) (key == GLFW_KEY_0 || key == GLFW_KEY_1 || key == GLFW_KEY_2 || \
					key == GLFW_KEY_3 || key == GLFW_KEY_4 || key == GLFW_KEY_5 || \
					key == GLFW_KEY_6 || key == GLFW_KEY_7 || key == GLFW_KEY_8 || \
					key == GLFW_KEY_9)
#define FVGLFW_NUMS_KEY_INT(key) (key - GLFW_KEY_0)

#define DEFAULT_WIN_WIDTH (1000)
#define DEFAULT_WIN_HEIGHT (1000)

#define UNIFORM_NAME_RESOLUTION "Resolution"
#define UNIFORM_NAME_MOUSE "Mouse"
#define UNIFORM_NAME_TIME "Time"
#define UNIFORM_NAME_PEAKAMP "PeakAmp"
#define UNIFORM_NAME_AVGAMP "AvgAmp"
#define NUM_COLORS 4
#define UNIFORM_NAME_COLOR1 "Color1"
#define UNIFORM_NAME_COLOR2 "Color2"
#define UNIFORM_NAME_COLOR3 "Color3"
#define UNIFORM_NAME_COLOR4 "Color4"

typedef float vec3[3];
