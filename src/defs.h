#pragma once

#define PCM_DEVICE "default"
#define MUSIC_BUFFER_SIZE (2048)

#define PEAK_ALPHA_ATTACK (0.9f)
#define PEAK_ALPHA_RELEASE (0.995f)
#define AVG_ALPHA (0.2f)

#define MUSIC_CONTROL_SLOW_SEC 2
#define MUSIC_CONTROL_FAST_SEC 5

#define PATH_SIZE (1024)
#define DATA_DIR ".FreeVisualizer"
#define SHADER_DIR "shaders"
#define SHADER_EXT ".glsl"

#define DEFAULT_WIN_WIDTH (1000)
#define DEFAULT_WIN_HEIGHT (1000)

#define UNIFORM_NAME_RESOLUTION "Resolution"
#define UNIFORM_NAME_MOUSE "Mouse"
#define UNIFORM_NAME_TIME "Time"
#define UNIFORM_NAME_PEAKAMP "PeakAmp"
#define UNIFORM_NAME_AVGAMP "AvgAmp"
