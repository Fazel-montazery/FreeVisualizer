#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define DT_REG 8

// Returns the home directory of the current user otherwise returns NULL
const char* getHomeDir(const bool verbose);

// Returns true if the directory at the path exist otherwise false
bool dirExists(const char* path, const bool verbose);

// Prints out all the regulat file names in a directory (widthout extention)
// returns ture if successful else false
bool printFilesInDir(const char* dirPath, const bool verbose);

// Returns true if file at the path exist otherwise false
bool fileExists(const char* path, const bool verbose);

// Pick a random file (non-uniform) in the (dirPath) dirctory and put into dest
// returns ture if successful else false
bool pickRandFile(const char* dirPath, char dest[], const size_t destsiz, const bool verbose);
