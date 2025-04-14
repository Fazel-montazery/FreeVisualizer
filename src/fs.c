#include "fs.h"

const char* getHomeDir(const bool verbose)
{
	const char* home = getenv("HOME");
	if (!home) {
		struct passwd* pw = getpwuid(getuid());
		if (!pw) {
			if (verbose)
				fprintf(stderr, "Couldn't retrive home directory: [%s]\n", strerror(errno));
			return NULL;
		}
		home = pw->pw_dir;
	}
	return home;
}

bool dirExists(const char* path, const bool verbose)
{
	struct stat s;
	if (stat(path, &s) == 0) {
		if (S_ISDIR(s.st_mode))
			return true;

		goto err;
	} else {
		goto err;
	}

err:
	if (verbose)
		fprintf(stderr, "Couldn't retrive directory: %s [%s]\n", path, strerror(errno));
	return false;
}

bool printFilesInDir(const char* dirPath, const bool verbose)
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
		if (verbose)
			fprintf(stderr, "Couldn't print directory: %s [%s]\n", dirPath, strerror(errno));
		return false;
	}
}

bool fileExists(const char* path, const bool verbose)
{
	struct stat s;
	if (stat(path, &s) == 0) {
		if (S_ISREG(s.st_mode))
			return true;

		goto err;
	} else {
		goto err;
	}

err:
	if (verbose)
		fprintf(stderr, "Couldn't retrive file: %s [%s]\n", path, strerror(errno));

	return false;
}

bool pickRandFile(const char* dirPath, char dest[], const size_t destsiz, const bool verbose)
{
	DIR *d;
	struct dirent *dir;
	d = opendir(dirPath);
	if (d) {
		srand(time(NULL));
		while ((dir = readdir(d))) {
			if (dir->d_name[0] == '.')
				continue;

			if (dir->d_type == DT_REG) {
				if (rand() % 2 == 1) {
					snprintf(dest, destsiz, "%s", dir->d_name);
					closedir(d);
					return true;
				}
			}
		}

		rewinddir(d);
		
		while ((dir = readdir(d))) {
			if (dir->d_name[0] == '.')
				continue;

			if (dir->d_type == DT_REG) {
				snprintf(dest, destsiz, "%s", dir->d_name);
				closedir(d);
				return true;
			}
		}

		closedir(d);
		goto err;
	} else {
		goto err;
	}

err:
	if (verbose)
		fprintf(stderr, "Couldn't retrive directory: %s [%s]\n", dirPath, strerror(errno));

	return false;
}
