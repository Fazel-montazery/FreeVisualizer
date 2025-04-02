#include "fs.h"

const char* getHomeDir()
{
	const char* home = getenv("HOME");
	if (!home) {
		struct passwd* pw = getpwuid(getuid());
		if (!pw) return NULL;
		home = pw->pw_dir;
	}
	return home;
}

static void _createDirInHome(const char* homeDir, const char* dirName, char dest[], size_t destsiz, int i)
{
	if (i == 2) {
		return;
	}

	snprintf(dest, destsiz, "%s/%s", homeDir, dirName);
	struct stat st;
	if (stat(dest, &st) == 0) {
		if (S_ISDIR(st.st_mode)) {
			return;
		} else {
			remove(dest);
			_createDirInHome(homeDir, dirName, dest, destsiz, i + 1);
			return;
		}
	} else {
		if (mkdir(dest, 0777) == 0) {
			return;
		} else {
			_createDirInHome(homeDir, dirName, dest, destsiz, i + 1);
			return;
		}
	}
}

void createDirInHome(const char* homeDir, const char* dirName, char dest[], size_t destsiz)
{
	_createDirInHome(homeDir, dirName, dest, destsiz, 0);
}

void printFilesInDir(const char* dirPath)
{
	DIR *d;
	struct dirent *dir;
	d = opendir(dirPath);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (dir->d_name[0] == '.')
				continue;

			if (dir->d_type == DT_REG) {
				char *last = strrchr(dir->d_name, '.');

				if (last)
					*last = '\0';

				printf("%s\n", dir->d_name);
			}
		}
		closedir(d);
	} else {
		fprintf(stderr, "Couldn't open directory: %s\n", dirPath);
	}
}
