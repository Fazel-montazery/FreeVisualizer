#include "shader.h"

static char* loadShaderSrc(const char* path)
{
	if (!path) {
		fprintf(stderr, "Path is NULL\n");
		return NULL;
	}

	FILE *file = fopen(path, "r");
	if (!file) {
		fprintf(stderr, "Failed to open file: [%s]\n", path);
		return NULL;
	}

	fseek(file, 0, SEEK_END);
	long length = ftell(file);
	if (length < 0) {
		fprintf(stderr, "Failed to determine the length of the file: [%s]\n", path);
		fclose(file);
		return NULL;
	}
	rewind(file);

	char* buffer = malloc(length + 1);
	if (!buffer) {
		fprintf(stderr, "Failed to allocate memory for ShaderSrc: [%s]\n", path);
		fclose(file);
		return NULL;
	}

	size_t read_length = fread(buffer, 1, length, file);
	if (read_length != length) {
		fprintf(stderr, "Failed to read the entire file: [%s]\n", path);
		fclose(file);
		free(buffer);
		return NULL;
	}

	fclose(file);
	buffer[length] = '\0';
	return buffer;
}

static void unLoadShaderSrc(char* str)
{
	if (str) free(str);
}

static bool compileShader(GLuint shader)
{
	glCompileShader(shader);
	int  success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		fprintf(stderr, "[ERROR::SHADER::COMPILATION_FAILED]\n%s\n", infoLog);
		return false;
	}
	return true;
}

bool createShaderFromPath(const char* path, GLuint* shader_id, GLenum shader_type)
{
	const char* shaderSrc = loadShaderSrc(path);
	if (!shaderSrc) {
		return false;
	}
	*shader_id = glCreateShader(shader_type);
	glShaderSource(*shader_id, 1, &shaderSrc, NULL);
	unLoadShaderSrc((char*)shaderSrc);
	if(!compileShader(*shader_id)) {
		return false;
	}
	return true;
}

bool createShaderFromSrc(const char* src, GLuint* shader_id, GLenum shader_type)
{
	*shader_id = glCreateShader(shader_type);
	glShaderSource(*shader_id, 1, &src, NULL);
	if(!compileShader(*shader_id)) {
		return false;
	}
	return true;
}


bool createProgrm(GLuint vertexShader, GLuint fragmentShader, GLuint* program_id)
{
	GLuint id = glCreateProgram();
	if (id == 0) return false;

	glAttachShader(id, vertexShader);
	glAttachShader(id, fragmentShader);
	glLinkProgram(id);

	int success;
	char infoLog[512];
	glGetProgramiv(id, GL_LINK_STATUS, &success);
	if(!success) {
		glGetProgramInfoLog(id, 512, NULL, infoLog);
		fprintf(stderr, "[ERROR::SHADER_PROGRAM::LINKING_FAILED]\n%s\n", infoLog);
		return false;
	}

	*program_id = id;

	return true;
}
