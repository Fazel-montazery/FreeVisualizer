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

#define DT_REG 8

// Returns the home directory of the current user otherwise returns NULL
const char* getHomeDir();

// Create the (dirName) directory in Home if It doesn't exsit otherwise just puts the full path into dest
// returns ture if successful else false
bool createDirInHome(const char* homeDir, const char* dirName, char dest[], size_t destsiz);

// Prints out all the regulat file names in a directory (widthout extention)
// returns ture if successful else false
bool printFilesInDir(const char* dirPath);

// Pick a random file in the (dirPath) dirctory and put into dest
// returns ture if successful else false
bool pickRandFile(const char* dirPath, char dest[], size_t destsiz);

// Returns true if file at the path exist otherwise false
bool fileExists(const char* path);
