#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../include/glad/glad.h"

char* loadShaderSrc(const char* path);
void unLoadShaderSrc(char* str);
bool compileShader(GLuint shader);
bool createShader(const char* path, GLuint* shader_id, GLenum shader_type);
bool createProgrm(GLuint vertexShader, GLuint fragmentShader, GLuint* program_id);
