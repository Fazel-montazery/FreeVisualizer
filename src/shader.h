#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "../include/glad/glad.h"

// Create a shader object from the file path provided. Returns true on success else false
bool createShaderFromPath(const char* path, GLuint* shader_id, const GLenum shader_type);

// Create a shader object from the Src provided. Returns true on success else false
bool createShaderFromSrc(const char* src, GLuint* shader_id, const GLenum shader_type);

// Crete the full programShader from previously created shaders
bool createProgrm(const GLuint vertexShader, const GLuint fragmentShader, GLuint* program_id);
