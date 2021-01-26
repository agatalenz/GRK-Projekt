#include "ParticleEmiter.h"
#include <cstdlib>

ParticleEmitter::ParticleEmitter(GLuint* program)
{
	this->program = program;

	this->positionsArr = new float[PARTICLES_COUNT * 4];

	particles.resize(PARTICLES_COUNT);
	for (int i = 0; i < PARTICLES_COUNT; ++i)
	{
		particles[i].position = glm::vec3(0);
		particles[i].lifetime = randomFloat(1.0f, 2.0f);
		particles[i].radius = 0.01f;
	}

	generateBuffers();
}

ParticleEmitter::ParticleEmitter(GLuint* program, int particleCount, float particleSize)
{
	this->program = program;

	this->positionsArr = new float[particleCount * 4];
	this->particleSize = particleSize;

	particles.resize(particleCount);
	for (int i = 0; i < particleCount; ++i)
	{
		particles[i].position = glm::vec3(0);
		particles[i].lifetime = randomFloat(1.0f, 2.0f);
		particles[i].radius = 0.01f;
	}

	generateBuffers();
}

void ParticleEmitter::generateBuffers()
{
	glGenBuffers(1, &particleVertexBuffer);

	std::vector< float > vertices;
	vertices.push_back(0.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
	vertices.push_back(1.0f); vertices.push_back(0.0f); vertices.push_back(0.0f);
	vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
	vertices.push_back(1.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);

	glBindBuffer(GL_ARRAY_BUFFER, particleVertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &particlePositionBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, particlePositionBuffer);
	glBufferData(GL_ARRAY_BUFFER, particles.size() * 4 * sizeof(float), positionsArr, GL_DYNAMIC_DRAW);
}

void ParticleEmitter::setupUniforms(const glm::mat4 transformation, glm::mat4 cameraMatrix, glm::mat4 perspectiveMatrix)
{
	glUniformMatrix4fv(glGetUniformLocation(*program, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(*program, "M_v"), 1, GL_FALSE, (float*)&cameraMatrix);
	glUniformMatrix4fv(glGetUniformLocation(*program, "M_p"), 1, GL_FALSE, (float*)&perspectiveMatrix);
	glUniform1f(glGetUniformLocation(*program, "particleSize"), this->particleSize);
}

void ParticleEmitter::update(const float dt, const glm::mat4 transformation, glm::mat4 cameraMatrix, glm::mat4 perspectiveMatrix)
{
	glUseProgram(*program);

	setupUniforms(transformation, cameraMatrix, perspectiveMatrix);


	for (int i = 0; i < particles.size(); ++i)
	{
		particles[i].lifetime -= dt;
		particles[i].radius += 0.0002;

		if (particles[i].lifetime <= 0.0f)
		{
			particles[i].position = glm::vec3(0);
			particles[i].lifetime = randomFloat(1.0f, 2.0f);
			particles[i].radius = 0.001f;
		}

		float radius = particles[i].radius;
		particles[i].position -= glm::vec3(randomFloat(-radius, radius), dt / 3, randomFloat(-radius, radius));

		positionsArr[i * 4 + 0] = particles[i].position[0];
		positionsArr[i * 4 + 1] = particles[i].position[1];
		positionsArr[i * 4 + 2] = particles[i].position[2];
		positionsArr[i * 4 + 3] = particles[i].lifetime;
	}
}

void ParticleEmitter::draw()
{
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(4);

	glBindBuffer(GL_ARRAY_BUFFER, particlePositionBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * 4 * sizeof(float), positionsArr);

	glBindBuffer(GL_ARRAY_BUFFER, particleVertexBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, particlePositionBuffer);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	glVertexAttribDivisor(4, 1);

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, particles.size());

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(4);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}

float ParticleEmitter::randomFloat(float min, float max) {
	return  (max - min) * ((((float)rand()) / (float)RAND_MAX)) + min;
}

ParticleEmitter::~ParticleEmitter()
{
	glDeleteBuffers(1, &particleVertexBuffer);
	glDeleteBuffers(1, &particlePositionBuffer);
}