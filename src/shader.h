#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../include/glad/glad.h"

bool createShaderFromPath(const char* path, GLuint* shader_id, GLenum shader_type);
bool createShaderFromSrc(const char* src, GLuint* shader_id, GLenum shader_type);
bool createProgrm(GLuint vertexShader, GLuint fragmentShader, GLuint* program_id);
