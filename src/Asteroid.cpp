#include "Asteroid.h"

Asteroid::Asteroid(glm::vec3 coordinates, GLuint texture, obj::Model model)
{
	Coordinates = glm::translate(coordinates);
	Texture = texture;
	Model = model;
}
