#pragma once
#include <vector>
#include <iostream>
#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
class Skybox
{
private:
	
public:
	unsigned static int loadCubemap(std::vector<std::string> faces);
	static void drawSkybox(GLuint program, glm::mat4 cameraMatrix, glm::mat4 perspectiveMatrix, GLuint texture);
};

