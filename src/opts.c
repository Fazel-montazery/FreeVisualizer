#include "opts.h"

#define OP_STRING "hs:lS:d:fp:tc:v:"

static const vec3 default_colors[NUM_COLORS] = {
	{0.610, 0.498, 0.650},
	{0.388, 0.498, 0.350},
	{0.530, 0.498, 0.620},
	{3.438, 3.012, 4.025}
};

static const struct option opts[] = {
	{"help", no_argument, 0, 'h'},
	{"scene", required_argument, 0, 's'},
	{"ls", no_argument, 0, 'l'},
	{"yt-search", required_argument, 0, 'S'},
	{"yt-dl", required_argument, 0, 'd'},
	{"fullscreen", no_argument, 0, 'f'},
	{"path", required_argument, 0, 'p'},
	{"test", no_argument, 0, 't'},
	{"color", required_argument, 0, 'c'},
	{"sub", required_argument, 0, 'v'},
	{0, 0, 0, 0}
};

#define SEARCH_COMMAND "yt-dlp --no-playlist ytsearch10:'%s official music' --get-title"
#define DOWNLOAD_COMMAND "yt-dlp -f bestaudio --no-playlist --extract-audio --audio-format mp3 --audio-quality 0 -o '%%(title)s' 'ytsearch: %s official music'"

static _Atomic bool loadingShouldExit;
static int loading(void *arg)
{
	const char loadingChars[] = {'/', '-', '\\'};
	int loadingCharIndx = 0;

	while (!atomic_load_explicit(&loadingShouldExit, memory_order_relaxed)) {
		fputc(loadingChars[loadingCharIndx % 3], stdout);
		fputc('\r', stdout);
		fflush(stdout);
		sleep(1);
		loadingCharIndx++;
	}

	return 0;
}

// Parse the colors in str and put them in c, else copy the default colors
// Yeah it's ugly :)
static void parseColors(const char* str, vec3 c[NUM_COLORS])
{
	int i = 0;
	char* colors = NULL;

	if (!str) goto final;

	colors = strdup(str);
	if (!colors) goto final;

	char* token = strtok(colors, " \t\n\v\f\r");
	while (token && i < NUM_COLORS) {
		if (strlen(token) != 7) {
			goto final;
		}

		char num[3] = { 0 };
		num[0] = token[1]; num[1] = token[2];
		c[i][0] = (strtol(num, NULL, 16) / 255.0);
		num[0] = token[3]; num[1] = token[4];
		c[i][1] = (strtol(num, NULL, 16) / 255.0);
		num[0] = token[5]; num[1] = token[6];
		c[i][2] = (strtol(num, NULL, 16) / 255.0);

		i++;
		token = strtok(NULL, " \t\n\v\f\r");
	}

final:
	while (i < NUM_COLORS) {
		c[i][0] = default_colors[i][0];
		c[i][1] = default_colors[i][1];
		c[i][2] = default_colors[i][2];
		i++;
	}

	if (colors) free(colors);
}

bool parseOpts( int argc, 
		char *argv[],
		char** musicPath,
		char* fragShaderPathBuf,
		size_t bufferSiz,
		bool* fullscreen,
		bool* testMode,
		vec3 colors[NUM_COLORS])
{
	const char* home = getHomeDir(true);
	if (!home)
		return false;

	char shaderDir[PATH_SIZE] = { 0 };
	snprintf(shaderDir, PATH_SIZE, "%s/%s/%s", home, DATA_DIR, SHADER_DIR);
	if (!dirExists(shaderDir, true))
		return false;

	bool sceneSet = false;
	int opt;
	int indx = 0;

	*fullscreen = false;

	// Setting default colors
	// I'm going for less code and using this to set default colors in case not specified
	parseColors(NULL, colors);

	while ((opt = getopt_long(argc, argv, OP_STRING, opts, &indx)) != -1) {
		switch (opt) {
		case 0:
			break;

		case 'h':
			printf("Usage: %s [OPTIONS] <mp3 file>\n\n"
					"Options:\n", argv[0]);
			printf("  %-20s%s\n", "-h, --help", "Print this help message");
			printf("  %-20s%s\n", "-s, --scene", "Which scene(shader) to use");
			printf("  %-20s%s\n", "-l, --ls", "List scenes");
			printf("  %-20s%s\n", "-S, --yt-search", "Search youtube and return 10 results");
			printf("  %-20s%s\n", "-d, --yt-dl", "Download the audio of a YouTube video by title");
			printf("  %-20s%s\n", "-f, --fullscreen", "Start in fullscreen mode");
			printf("  %-20s%s\n", "-p, --path", "Use the shader at path");
			printf("  %-20s%s\n", "-t, --test", "No audio mode, just for testing out the shaders");
			printf("  %-20s%s\n", "-c, --color", 
				"use Custom colors in the string: \"#FF0000 #0000FF #00FF00 $FFFFFF\" (max 4)");
			return false;

		case 's':
			char scenePath[PATH_SIZE] = { 0 };
			snprintf(scenePath, PATH_SIZE, "%s/%s%s", shaderDir, optarg, SHADER_EXT);

			if (!fileExists(scenePath, false)) {
				printf("Scene Doesn't exist!\n"
					"Available scenes:\n");

				if (!printFilesInDir(shaderDir, false)) {
					printf("Couldn't list scenes: %s\n", strerror(errno));
					return false;
				}
				return false;
			}

			snprintf(fragShaderPathBuf, bufferSiz, "%s", scenePath);
			sceneSet = true;

			break;

		case 'l':
			printFilesInDir(shaderDir, true);
			return false;

		case 'S':
			// Setting exit condition
			atomic_store_explicit(&loadingShouldExit, false, memory_order_relaxed);

			// Hide terminal cursor
			printf("\033[?25l");
			fflush(stdout);

			thrd_t loadingThread;
			thrd_create(&loadingThread, loading, NULL);

			char search[PATH_SIZE] = { 0 };
			snprintf(search, PATH_SIZE, SEARCH_COMMAND, optarg);

			if (system(search) == -1) {
				printf("Couldn't search youtube: %s\n", strerror(errno));
			}

			atomic_store_explicit(&loadingShouldExit, true, memory_order_relaxed);
			thrd_join(loadingThread, NULL);
			
			// Show terminal cursor
			printf("\033[?25h");
			fflush(stdout);
			return false;

		case 'd':
			char download[PATH_SIZE] = { 0 };
			snprintf(download, PATH_SIZE, DOWNLOAD_COMMAND, optarg);

			if (system(download) == -1) {
				printf("Couldn't download from youtube: %s\n", strerror(errno));
			}

			return false;

		case 'f':
			*fullscreen = true;
			break;

		case 'p':
			snprintf(fragShaderPathBuf, bufferSiz, "%s", optarg);
			sceneSet = true;
			break;

		case 't':
			*testMode = true;
			break;

		case 'c':
			if (optarg)
				parseColors(optarg, colors);
			break;

		case 'v':
			if (optarg)
				process_srt(optarg);
			break;

		case '?':
			return false;

		default:
			return false;
		}
	}

	if (!sceneSet) {
		char sceneName[PATH_SIZE / 4];
		if (!pickRandFile(shaderDir, sceneName, PATH_SIZE / 4, false)) {
			fprintf(stderr, "No scene found!\n");
			return false;
		}
		snprintf(fragShaderPathBuf, PATH_SIZE, "%s/%s", shaderDir, sceneName);
	}

	if (*testMode) return true;

	if (optind < argc) {
		*musicPath = argv[optind];
	} else {
		printf("Usage: %s [OPTIONS] <mp3 file>\n"
			"run '%s -h' for help\n", argv[0], argv[0]);
		return false;
	}

	return true;
}
