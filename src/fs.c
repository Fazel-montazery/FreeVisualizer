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

bool dirExists(const char* path)
{
	struct stat s;
	if (stat(path, &s) == 0) {
		if (S_ISDIR(s.st_mode))
			return true;

		return false;
	} else {
		return false;
	}
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
		return false;
	} else {
		fprintf(stderr, "Couldn't open directory: %s\n", dirPath);
		return false;
	}

}
