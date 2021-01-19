#pragma once

#include "glew.h"
#include "freeglut.h"
#include <iostream>

namespace Core
{

	class Shader_Loader
	{
	private:

		std::string ReadShader(char *filename);
		GLuint CreateShader(GLenum shaderType,
			std::string source,
			char* shaderName);

	public:

		Shader_Loader(void);
		~Shader_Loader(void);
		GLuint CreateProgram(char* VertexShaderFilename,
			char* FragmentShaderFilename);
		GLuint CreateProgram(char* vertexShaderFilename,
			char* fragmentShaderFilename, char* geometryShaderFilename);

		void DeleteProgram(GLuint program);

	};
}