#pragma once

#include "glm.hpp"
#include "objload.h"
#include <ext.hpp>
#include <glew.h>
#include <string>
#include <iostream>
#include <filesystem>
#include <ctime>
#include <vector>

class Asteroid
{
public:

	GLuint Texture;
	obj::Model Model;
	glm::mat4 Coordinates;

	Asteroid::Asteroid(glm::vec3 coordinates, GLuint texture, obj::Model model);
};