#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <dirent.h>

#define DT_REG 8

const char* getHomeDir();
void createDirInHome(const char* homeDir, const char* dirName, char dest[], size_t destsiz);
void printFilesInDir(const char* dirPath);
