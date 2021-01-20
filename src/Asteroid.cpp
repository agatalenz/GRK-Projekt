#include "Asteroid.h"

Asteroid::Asteroid(glm::vec3 coordinates, GLuint texture, obj::Model model)
{
	Coordinates = glm::translate(coordinates) * glm::scale(glm::vec3(GetRandomFloat(0.5, 2.5)));
	Texture = texture;
	Model = model;
}

float GetRandomFloat(float from, float to) {

	float random = ((float)rand()) / (float)RAND_MAX;
	float r = random * (to - from);
	return from + r;
}