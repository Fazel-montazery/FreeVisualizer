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

static bool _createDirInHome(const char* homeDir, const char* dirName, char dest[], size_t destsiz, int i)
{
	if (i == 2) {
		return false;
	}

	snprintf(dest, destsiz, "%s/%s", homeDir, dirName);
	struct stat st;
	if (stat(dest, &st) == 0) {
		if (S_ISDIR(st.st_mode)) {
			return true;
		} else {
			remove(dest);
			_createDirInHome(homeDir, dirName, dest, destsiz, i + 1);
		}
	} else {
		if (mkdir(dest, 0777) == 0) {
			return true;
		} else {
			_createDirInHome(homeDir, dirName, dest, destsiz, i + 1);
		}
	}

	return false;
}

bool createDirInHome(const char* homeDir, const char* dirName, char dest[], size_t destsiz)
{
	return _createDirInHome(homeDir, dirName, dest, destsiz, 0);
}

bool printFilesInDir(const char* dirPath)
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
		return true;
	} else {
		fprintf(stderr, "Couldn't open directory: %s\n", dirPath);
		return false;
	}
}

bool fileExists(const char* path)
{
	struct stat s;
	if (stat(path, &s) == 0) {
		if (S_ISREG(s.st_mode))
			return true;

		return false;
	} else {
		return false;
	}
}

bool pickRandFile(const char* dirPath, char dest[], size_t destsiz)
{
	DIR *d;
	struct dirent *dir;
	d = opendir(dirPath);
	if (d) {
		while ((dir = readdir(d))) {
			if (dir->d_name[0] == '.')
				continue;

			if (dir->d_type == DT_REG) {
				srand(time(NULL));
				if (rand() % 2) {
					snprintf(dest, destsiz, "%s", dir->d_name);
					closedir(d);
					return true;
				}
			}
		}
		rewinddir(d);
		if ((dir = readdir(d))) {
			snprintf(dest, destsiz, "%s", dir->d_name);
		}
		closedir(d);
		fprintf(stderr, "No scene found!\n");
		return false;
	} else {
		fprintf(stderr, "Couldn't open directory: %s\n", dirPath);
		return false;
	}

}
