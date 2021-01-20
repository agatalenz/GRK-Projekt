#pragma once

#include "Texture.h"
#include "glm.hpp"
#include "objload.h"
#include <glew.h>
#include <ext.hpp>
#include <filesystem>

namespace fs = std::filesystem;

class Asteroid
{

public:
	GLuint Texture;
	obj::Model Model;
	glm::mat4 Coordinates;

	Asteroid::Asteroid(glm::vec3 coordinates, GLuint texture, obj::Model model);
};

float GetRandomFloat(float from, float to);