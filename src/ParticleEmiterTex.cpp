#include "ParticleEmiterTex.h"
#include <cstdlib>
#include "Texture.h"

//ParticleEmitterTex::ParticleEmitterTex(GLuint* program)
//{
//	this->program = program;
//
//	this->positionsArr = new float[PARTICLES_COUNT * 4];
//
//	particles.resize(PARTICLES_COUNT);
//	for (int i = 0; i < PARTICLES_COUNT; ++i)
//	{
//		particles[i].position = glm::vec3(0);
//		particles[i].lifetime = randomFloat(1.0f, 2.0f);
//		particles[i].radius = 0.01f;
//	}
//
//	generateBuffers();
//}

ParticleEmitterTex::ParticleEmitterTex(GLuint* program, int particleCount, float particleSize, GLuint texId)
{
	this->program = program;

	this->positionsArr = new float[particleCount * 4];
	this->particleSize = particleSize;
	this->texId = texId;

	particles.resize(particleCount);
	for (int i = 0; i < particleCount; ++i)
	{
		particles[i].position = glm::vec3(0);
		particles[i].velocity = glm::normalize(glm::vec3(randomFloat(-2, 2), randomFloat(-2, 2), randomFloat(-2, 2)));
		particles[i].lifetime = randomFloat(1.0f, 2.0f);
		particles[i].radius = 0.02f;
	}

	generateBuffers();
}

void ParticleEmitterTex::generateBuffers()
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

	glGenBuffers(1, &particleTexBuffer);
	std::vector<float> texCoord;
	texCoord.push_back(0.0f); texCoord.push_back(0.0f);
	texCoord.push_back(0.0f); texCoord.push_back(1.0f);
	texCoord.push_back(1.0f); texCoord.push_back(0.0f);
	texCoord.push_back(1.0f); texCoord.push_back(1.0f);
	glBindBuffer(GL_ARRAY_BUFFER, particleTexBuffer);
	glBufferData(GL_ARRAY_BUFFER, texCoord.size() * sizeof(float), texCoord.data(), GL_STATIC_DRAW);
}

void ParticleEmitterTex::setupUniforms(const glm::mat4 transformation, glm::mat4 cameraMatrix, glm::mat4 perspectiveMatrix)
{
	glUniformMatrix4fv(glGetUniformLocation(*program, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(*program, "M_v"), 1, GL_FALSE, (float*)&cameraMatrix);
	glUniformMatrix4fv(glGetUniformLocation(*program, "M_p"), 1, GL_FALSE, (float*)&perspectiveMatrix);
	glUniform1f(glGetUniformLocation(*program, "particleSize"), this->particleSize);
	Core::SetActiveTexture(this->texId, "textureSampler", *this->program, 0);
}

float clamp(float min, float max, float a){
	if (a > max) return max;
	if (a < min) return min;
	return a;
}

void ParticleEmitterTex::update(const float dt, const glm::mat4 transformation, glm::mat4 cameraMatrix, glm::mat4 perspectiveMatrix)
{
	glUseProgram(*program);

	setupUniforms(transformation, cameraMatrix, perspectiveMatrix);


	for (int i = 0; i < particles.size(); ++i)
	{
		particles[i].lifetime -= dt*3;
		//particles[i].radius += 0.0075;
		//this->particleSize -= 0.000005f;
		//ethis->particleSize = clamp(0.0, this->particleSize, this->particleSize - 0.000001f);
		this->particleSize *= 0.999985f;
		glUniform1f(glGetUniformLocation(*program, "particleSize"), this->particleSize);

		/*if (particles[i].lifetime <= 0.0f)
		{
			particles[i].position = glm::vec3(0);
			particles[i].lifetime = randomFloat(1.0f, 2.0f);
			particles[i].radius = 0.02f;
		}*/

		float radius = particles[i].radius;
		//particles[i].position -= glm::vec3(randomFloat(-radius, radius), randomFloat(-radius, radius), randomFloat(-radius, radius));
		particles[i].position += particles[i].velocity * 0.035f;

		positionsArr[i * 4 + 0] = particles[i].position[0];
		positionsArr[i * 4 + 1] = particles[i].position[1];
		positionsArr[i * 4 + 2] = particles[i].position[2];
		positionsArr[i * 4 + 3] = particles[i].lifetime;
	}
}

void ParticleEmitterTex::draw()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_NOTEQUAL, 0.0);
	

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);

	glBindBuffer(GL_ARRAY_BUFFER, particlePositionBuffer);
	glBufferSubData(GL_ARRAY_BUFFER, 0, particles.size() * 4 * sizeof(float), positionsArr);

	glBindBuffer(GL_ARRAY_BUFFER, particleVertexBuffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, particlePositionBuffer);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
	glVertexAttribDivisor(4, 1);

	glBindBuffer(GL_ARRAY_BUFFER, particleTexBuffer);
	glVertexAttribPointer(5, 2, GL_FLOAT, false, 0, nullptr);

	glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, particles.size());

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(4);
	glDisableVertexAttribArray(5);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);
}

float ParticleEmitterTex::randomFloat(float min, float max) {
	return  (max - min) * ((((float)rand()) / (float)RAND_MAX)) + min;
}

ParticleEmitterTex::~ParticleEmitterTex()
{
	glDeleteBuffers(1, &particleVertexBuffer);
	glDeleteBuffers(1, &particlePositionBuffer);
	glDeleteBuffers(1, &particleTexBuffer);
}