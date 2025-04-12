#include "opts.h"

#define OP_STRING "hs:lS:d:f"

static const struct option opts[] = {
	{"help", no_argument, 0, 'h'},
	{"scene", required_argument, 0, 's'},
	{"ls", no_argument, 0, 'l'},
	{"yt-search", required_argument, 0, 'S'},
	{"yt-dl", required_argument, 0, 'd'},
	{"fullscreen", no_argument, 0, 'f'},
	{0, 0, 0, 0}
};

#define SEARCH_COMMAND "yt-dlp --no-playlist ytsearch10:'%s official music' --get-title"
#define DOWNLOAD_COMMAND "yt-dlp -f bestaudio --no-playlist --extract-audio --audio-format mp3 --audio-quality 0 -o '%%(title)s' 'ytsearch: %s official music'"

static const char loadingChars[] = {'/', '-', '\\'};
static int loadingCharIndx = 0;
static volatile bool loading_should_exit = false;
static void* loading(void *data)
{
	loading_should_exit = false;
	while (!loading_should_exit) {
		fputc(loadingChars[loadingCharIndx % 3], stdout);
		fputc('\r', stdout);
		fflush(stdout);
		sleep(1);
		loadingCharIndx++;
	}
	return NULL;
}

bool parseOpts( int argc, 
		char *argv[],
		char** musicPath,
		char* fragShaderPathBuf,
		size_t bufferSiz,
		bool* fullscreen
)
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
			// Hide terminal cursor
			printf("\033[?25l");
			fflush(stdout);

			pthread_t loadingThread;
			pthread_create(&loadingThread, NULL, loading, NULL);

			char search[PATH_SIZE] = { 0 };
			snprintf(search, PATH_SIZE, SEARCH_COMMAND, optarg);

			if (system(search) == -1) {
				printf("Couldn't search youtube: %s\n", strerror(errno));
			}

			loading_should_exit = true;
			pthread_join(loadingThread, NULL);
			
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

		case '?':
			return false;

		default:
			return false;
		}
	}

	if (optind < argc) {
		*musicPath = argv[optind];
	} else {
		printf("Usage: %s [OPTIONS] <mp3 file>\n"
			"run '%s -h' for help\n", argv[0], argv[0]);
		return false;
	}

	if (!sceneSet) {
		char sceneName[PATH_SIZE / 4];
		if (!pickRandFile(shaderDir, sceneName, PATH_SIZE / 4, false)) {
			fprintf(stderr, "No scene found!\n");
			return false;
		}
		snprintf(fragShaderPathBuf, PATH_SIZE, "%s/%s", shaderDir, sceneName);
	}

	if (optind < argc) {
		*musicPath = argv[optind];
	} else {
		printf("Usage: %s [OPTIONS] <mp3 file>\n"
			"run '%s -h' for help\n", argv[0], argv[0]);
		return false;
	}

	return true;
}
