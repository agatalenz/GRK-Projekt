#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include <vector>


#define PARTICLES_COUNT 100

class ParticleEmitterTex
{
public:
	ParticleEmitterTex(GLuint* program);
	ParticleEmitterTex(GLuint* program, int particleCount, float particleSize, GLuint texId);
	~ParticleEmitterTex();

	void update(const float dt, const glm::mat4 transformation, glm::mat4 cameraMatrix, glm::mat4 perspectiveMatrix);
	void draw();

private:
	struct Particle
	{
		glm::vec3 position;
		glm::vec3 velocity;
		float lifetime = 0.0f;
		float radius = 0.0f;
	};

	float* positionsArr;
	float particleSize = 0.015f;
	GLuint* program;
	GLuint particleVertexBuffer;
	GLuint particlePositionBuffer;
	GLuint particleTexBuffer;
	GLuint texId;
	std::vector< Particle > particles;

	float randomFloat(float min, float max);
	void generateBuffers();
	void setupUniforms(const glm::mat4 transformation, glm::mat4 cameraMatrix, glm::mat4 perspectiveMatrix);
};