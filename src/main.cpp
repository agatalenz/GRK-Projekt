#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include "Skybox.h"
#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"
#include "Texture.h"
#include "Physics.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Asteroid.h"
#include <irrKlang.h>
#include "ParticleEmiter.h"
#include "ParticleEmiterTex.h"

using namespace std;
using namespace irrklang;

//#pragma comment(lib, "irrKlang.lib") 
ISoundEngine* SoundEngine = createIrrKlangDevice();

//------------------------------------------------------------------
// PARTICLES

// obj
ParticleEmitter* particleEmitter_LeftEngine;
ParticleEmitter* particleEmitter_RightEngine;
ParticleEmitterTex* particleEmitter_ShipExplode;
ParticleEmitterTex* particleEmitter_AstExplode;

// offset
const glm::vec3 engineOffset = glm::vec3(-0.23f, -0.08f, -0.4f);
const glm::mat4 engineRotation = glm::rotate(glm::radians(90.0f), glm::vec3(1, 0, 0));

// particle program
GLuint programEngineParticle;
GLuint explosionTexture;

//------------------------------------------------------------------
// RAYCASTING

Core::RayContext rayContext;
std::vector<glm::vec3> ray;
bool isShooting;

//------------------------------------------------------------------
// PROGRAM

int windowWidth = 600;
int windowHeight = 600;

GLuint programColor;
GLuint programTexture;
GLuint programCubemap;
GLuint programSkybox;
GLuint programStatic;
GLuint programExplode;
GLuint programTextureParticle;
GLuint programRed;

GLuint winHandle;

const float RADIUS = 100.f;
const int ASTEROIDS_NUMBER = 100;
int ACTUAL_ASTEROIDS_NUMBER = 0;
float asteroidAcceleration = 0.05f;
int asteroidsDestroyed = 0;
glm::mat4 kaboomAstPos;
bool isKaboom = false;

Core::Shader_Loader shaderLoader;
obj::Model shipModel, sphereModel, gemModel;
std::vector<PxRigidDynamic*> asteroidsBodies;
std::vector<glm::vec3> astPositions;

// textures

GLuint textureShip;
GLuint textureShipNormal;
GLuint texturePlanet;
GLuint textureGem;
std::vector<GLuint> texturePlanetAnimated;
std::vector<GLuint> asteroidTextures;

// skybox

GLuint cubemapTexture;
GLuint skyboxVAO, skyboxVBO;

std::vector<std::string> faces
{
	"textures/skybox/space.jpg",
	"textures/skybox/space.jpg",
	"textures/skybox/space.jpg",
	"textures/skybox/space.jpg",
	"textures/skybox/space.jpg",
	"textures/skybox/space.jpg",
};

// camera

glm::vec3 cameraPos = glm::vec3(0, 0, 0);
glm::vec3 cameraDir;	// Wektor "do przodu" kamery
glm::vec3 cameraSide;	// Wektor "w bok" kamery
float cameraAngle = 0;

glm::mat4 cameraMatrix, perspectiveMatrix;
glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, -0.9f, -1.0f));
glm::quat rotation = glm::quat(1, 0, 0, 0);
glm::quat rotationZ = glm::angleAxis(glm::radians(0.f), glm::vec3(0, 0, 1));

bool firstMouse;
float newMouseX = 0;
float newMouseY = 0;
float lastX = 0;
float lastY = 0;
float mouseSpeed = 0.01f;

glm::mat4 createCameraMatrix()
{
	glm::quat rotationChange;
	rotationChange = glm::angleAxis(float(newMouseY) * mouseSpeed, glm::vec3(1.f, 0.f, 0.f)) * glm::angleAxis(float(newMouseX) * mouseSpeed, glm::vec3(0.f, 1.f, 0.f));
	newMouseX = 0;
	newMouseY = 0;
	rotation = rotationChange * rotation;
	rotation = normalize(rotation);
	cameraDir = glm::inverse(rotation) * glm::vec3(0, 0, -1.f);
	cameraSide = glm::inverse(rotation) * glm::vec3(1.f, 0, 0);

	return Core::createViewMatrixQuat(cameraPos, rotation);
}

// physical objects

PxRigidStatic *asteroidBody = nullptr;
PxRigidDynamic* shipBody = nullptr;
PxRigidStatic* gemBody = nullptr;
PxMaterial *asteroidMaterial = nullptr;
PxMaterial* shipMaterial = nullptr;
PxMaterial* gemMaterial = nullptr;

Core::RenderContext shipContext;
Core::RenderContext gemContext;
vector<Core::RenderContext> asteroidContexts;

// renderable objects (description of a single renderable instance)
struct Renderable {
	Core::RenderContext* context;
	glm::mat4 modelMatrix;
	GLuint textureId;
};
std::vector<Renderable*> renderables;
std::vector<Renderable*> renderablesAsteroids;

int amountHp, amountArmor, amountEngines, amountSources;
bool engineON;
float maxSpeed = 10;
float acceleration = 0;
float accelerationSpeed = 0.1f;

bool explode = false;
float expl_time = 0.0f;
float expl_speed = 0.03f;

bool canShoot = true;

//------------------------------------------------------------------
// SPACESHIP

// speed

void speedUp() {

	if (acceleration < maxSpeed) {
		acceleration += accelerationSpeed;
	}
	else {
		acceleration = maxSpeed;
	}

	PxVec3 velocity = PxVec3(cameraDir.x, cameraDir.y, cameraDir.z) * acceleration;
	shipBody->setLinearVelocity(velocity);
}

void slowDown() {

	if (acceleration > 0) {
		acceleration -= accelerationSpeed;
	}
	else {
		acceleration = 0;
	}

	PxVec3 velocity = PxVec3(cameraDir.x, cameraDir.y, cameraDir.z) * acceleration;
	shipBody->setLinearVelocity(velocity);
}

void changeFlightDir() {

	PxVec3 velocity = PxVec3(cameraDir.x, cameraDir.y, cameraDir.z) * acceleration;
	shipBody->setLinearVelocity(velocity);
}

// engines

void enableEngines() {
	if (!engineON) {
		particleEmitter_LeftEngine = new ParticleEmitter(&programEngineParticle, 5000, 0.0030f);
		particleEmitter_RightEngine = new ParticleEmitter(&programEngineParticle, 5000, 0.0030f);
		engineON = true;
	}
}

void disableEngines() {
	if (engineON) {
		particleEmitter_LeftEngine->~ParticleEmitter();
		particleEmitter_RightEngine->~ParticleEmitter();
		engineON = false;
	}
}

void onHit() {

	switch (amountArmor) {
		case 1: {
			if (amountHp > 0) {
				amountHp -= 3;
			}
			break;
		}
		case 2: {
			if (amountHp > 0) {
				amountHp -= 2;
			}
			break;
		}
		case 3: {
			if (amountHp > 0) {
				amountHp -= 1;
			}
			break;
		}
	}
		
	if(amountHp <= 0) {
		amountHp = 0;
		explode = true; 
		acceleration = 0;
		canShoot = false;
		particleEmitter_ShipExplode = new ParticleEmitterTex(&programTextureParticle, 2000, 0.05f, explosionTexture); 
		disableEngines();
	}
}

void addCash() {

	amountSources++;
}

//------------------------------------------------------------------
// CONTACT

// contact pairs filtering function
static PxFilterFlags simulationFilterShader(PxFilterObjectAttributes attributes0,
	PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	pairFlags =
		PxPairFlag::eCONTACT_DEFAULT | // default contact processing
		PxPairFlag::eNOTIFY_CONTACT_POINTS | // contact points will be available in onContact callback
		PxPairFlag::eNOTIFY_TOUCH_FOUND; // onContact callback will be called for this pair
		PxPairFlag::eNOTIFY_TOUCH_PERSISTS;

	return physx::PxFilterFlag::eDEFAULT;
}

// simulation events processor
class SimulationEventCallback : public PxSimulationEventCallback
{
public:
	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
	{
		
		if (pairHeader.actors[0]->userData == renderables[0] && pairHeader.actors[1]->userData != renderables[0]) {

			//if (count(renderables.begin(), renderables.end(), pairHeader.actors[1]->userData)) {
			Renderable* renderable = (Renderable*)pairHeader.actors[1]->userData;

			if(renderable->context == &gemContext){

				int i = count(renderables.begin(), renderables.end(), renderable);

				PxVec3 gemPos = pairHeader.actors[1]->getGlobalPose().p;
				float distance = glm::distance(cameraPos, glm::vec3(gemPos.x, gemPos.y, gemPos.z));

				if (i != 0 && distance < 7.f) {

					renderables.erase(renderables.begin() + i, renderables.begin() + i + 1);
					cout << "renderables size: " << renderables.size() << ", erase at: " << i << endl;

					//cout << "- gem at: ( " << position.x << ", " << position.y << ", " << position.z << ")" << endl;
					addCash();
				}
			}
			else {

				onHit();
			}
		}

		
		//for (PxU32 i = 0; i < nbPairs; i++)
		//{
		//	const PxContactPair& cp = pairs[i];

		//	PxU32 bufferSize = cp.contactCount;
		//	PxContactPairPoint* buffer = new PxContactPairPoint[bufferSize];
		//	cp.extractContacts(buffer, bufferSize);

		//	for (PxU32 i = 0; i < bufferSize; i++)
		//	{
		//		cout << "(x, y, z) = (" << buffer[i].position.x << ", " << buffer[i].position.y << ", " << buffer[i].position.z << ")" << std::endl;
		//	}
		//}
	}

	virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) {}
	virtual void onWake(PxActor** actors, PxU32 count) {}
	virtual void onSleep(PxActor** actors, PxU32 count) {}
	virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) {}
	virtual void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) {}
};

// PhysX init
SimulationEventCallback simulationEventCallback;
Physics pxScene(0.f, simulationFilterShader, &simulationEventCallback);

// fixed timestep for stable and deterministic simulation
const double physicsStepTime = 1.f / 60.f;
double physicsTimeToProcess = 0;

void enableEngines();
void disableEngines();

PxVec3 vec3ToPxVec(glm::vec3 vector) {
	return PxVec3(vector.x, vector.y, vector.z);
}
glm::vec3 PxVecTovec3(PxVec3 vector) {
	return glm::vec3(vector.x, vector.y, vector.z);
}

void generateGem(float x, float y, float z) {

	Renderable* gem = new Renderable();
	gem->context = &gemContext;
	gem->textureId = textureGem;
	renderables.emplace_back(gem);

	gemMaterial = pxScene.physics->createMaterial(1, 1, 1);
	gemBody = pxScene.physics->createRigidStatic(PxTransform(x, y, z));
	PxShape* gemShape = pxScene.physics->createShape(PxBoxGeometry(1, 1, 1), *gemMaterial);
	gemBody->attachShape(*gemShape);
	gemShape->release();
	gemBody->userData = gem;
	pxScene.scene->addActor(*gemBody);

	cout << "+ gem at: ( " << x << ", " << y << ", " << z << ")" << endl;

}

void upArmor() {

	if (amountArmor < 4) {

		int currentBalance = amountSources;
		switch (amountArmor) {
		case 1:
			if (currentBalance >= 10) {
				amountArmor++;
				currentBalance -= 10;
			};
			break;
		case 2:
			if (currentBalance >= 20) {
				amountArmor++;
				currentBalance -= 20;
			};
			break;
		case 3:
			if (currentBalance >= 30) {
				amountArmor++;
				currentBalance -= 30;
			};
			break;
		}
		amountSources = currentBalance;
	}
}

void upEngines() {

	if (amountEngines < 4) {

		int currentBalance = amountSources;
		switch (amountEngines) {
		case 1:
			if (currentBalance >= 10) {
				amountEngines++;
				currentBalance -= 10;
				maxSpeed = 12;
				accelerationSpeed = 0.15f;
			};
			break;
		case 2:
			if (currentBalance >= 20) {
				amountEngines++;
				currentBalance -= 20;
				maxSpeed = 15;
				accelerationSpeed = 0.2f;
			};
			break;
		case 3:
			if (currentBalance >= 30) {
				amountEngines++;
				currentBalance -= 30;
				maxSpeed = 20;
				accelerationSpeed = 0.3f;
			};
			break;
		}
		amountSources = currentBalance;
	}
}


std::vector<glm::vec3> calculate_ray(float x, float y) {

	glm::vec2 screen_space_pos(x, y);
	std::vector<glm::vec3> result;

	//here ray should be calculated
	glm::vec4 start = glm::vec4(screen_space_pos.x, screen_space_pos.y, -1, 1);
	start -= glm::vec4(0,1.f,0,0);

	glm::vec4 end = glm::vec4(screen_space_pos.x, screen_space_pos.y, 1, 1);	

	start = glm::inverse(perspectiveMatrix) * start;
	end = glm::inverse(perspectiveMatrix) * end;

	start = start / start.w;
	end = end / end.w;

	start = glm::inverse(cameraMatrix) * start;
	end = glm::inverse(cameraMatrix) * end;

	result.push_back(start);
	result.push_back(glm::normalize(end - start));

	return result;
}

void updateRay() {
	float x = 0.f;
	float y = 0.f;

	ray = calculate_ray(x, y);
	Core::updateRayPos(rayContext, ray);
}


void asteroidGoesKaboom(glm::mat4 currentAstPos)
{
	particleEmitter_AstExplode->update(0.01f, currentAstPos, cameraMatrix, perspectiveMatrix);
	particleEmitter_AstExplode->draw();
}

//----------------------------------------------
// DRAWING FUNCTIONS

void drawObjectColor(obj::Model* model, glm::mat4 modelMatrix, glm::vec3 color)
{
	GLuint program = programColor;

	glUseProgram(program);

	glUniform3f(glGetUniformLocation(program, "objectColor"), color.x, color.y, color.z);
	glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	Core::DrawModel(model);

	glUseProgram(0);
}

void drawObjectTexture(obj::Model* model, glm::mat4 modelMatrix, GLuint textureId)
{
	GLuint program = programTexture;

	glUseProgram(program);

	glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
	glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	Core::SetActiveTexture(textureId, "textureSampler", program, 0);
	//Core::SetActiveTexture(normalmapId, "normalSampler", program, 1);

	Core::DrawModel(model);

	glUseProgram(0);
}

void drawObjectTextureFromContext(Core::RenderContext* context, glm::mat4 modelMatrix, GLuint textureId)
{
	GLuint program = programTexture;

	glUseProgram(program);

	glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
	Core::SetActiveTexture(textureId, "textureSampler", program, 0);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	Core::DrawContext(*context);

	glUseProgram(0);
}

void drawObjectExplode(Core::RenderContext* context, glm::mat4 modelMatrix, GLuint textureId)
{
	GLuint program = programExplode;

	glUseProgram(program);

	//glUniform3f(glGetUniformLocation(program, "objectColor"), color.x, color.y, color.z);
	glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
	Core::SetActiveTexture(textureId, "textureSampler", program, 0);
	glUniform1f(glGetUniformLocation(program, "time"), expl_time);
	expl_time += expl_speed;

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	Core::DrawContext(*context);

	glUseProgram(0);
}

void printShop(std::string text, int x, int y) {

	glDisable(GL_TEXTURE_2D); //added this
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	gluOrtho2D(0.0, windowWidth, 0.0, windowHeight);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glWindowPos2i(0.01 * x * windowWidth, 0.01 * y * windowHeight);
	std::string s = text;
	void* font = GLUT_BITMAP_TIMES_ROMAN_24;
	for (std::string::iterator i = s.begin(); i != s.end(); ++i)
	{
		char c = *i;
		glutBitmapCharacter(font, c);
	}
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glEnable(GL_TEXTURE_2D);
}

void drawHealth(float health) {

	const int numDiv = 15;
	const float sep = 0.02f;
	const float barHeight = 0.5f / (float)numDiv;
	glBegin(GL_QUADS);
	for (float i = 0; i < health; i += (sep + barHeight)) {
		glVertex2f(0, i);
		glVertex2f(0.08f, i);
		glVertex2f(0.08f, i + barHeight);
		glVertex2f(0, i + barHeight);
	}
	glEnd();
}

float simple(float n) {
	//żeby tekst i paski dało się w takiej samej konwencji
	return (-1.f + 0.02f * n);
};

void drawStaticScene(int hp, int weapon, int armor, int sources) {
	std::map<int, float> bar = {
	{ 1, 0.05f },
	{ 2, 0.1f },
	{ 3, 0.15f },
	{ 4, 0.2f },
	{ 5, 0.25f },
	{ 6, 0.3f },
	{ 7, 0.35f },
	{ 8, 0.4f },
	{ 9, 0.45f },
	{ 10, 0.5f }
	};
	if (weapon > 4) {
		weapon = 4;
	};
	if (armor > 4) {
		armor = 4;
	};
	if (hp > 10) {
		hp = 10;
	};
	float _hp = bar[hp];
	float _weapon = bar[weapon];
	float _armor = bar[armor];
	glm::vec3 translateVec = glm::vec3(simple(94), simple(3), 0.f);
	glUniformMatrix4fv(glGetUniformLocation(programStatic, "transformation"),
		1, GL_FALSE, (float*)&glm::translate(translateVec));

	glm::vec3 color = glm::vec3(255, 0, 0);
	glUniform3f(glGetUniformLocation(programStatic, "objectColor"), color.x, color.y, color.z);
	drawHealth(_hp);
	color = glm::vec3(255, 255, 0);
	glUniform3f(glGetUniformLocation(programStatic, "objectColor"), color.x, color.y, color.z);

	printShop("[1] Engines:", 1, 88);
	translateVec = glm::vec3(simple(9.5f), simple(88), 0.f);
	glUniformMatrix4fv(glGetUniformLocation(programStatic, "transformation"),
		1, GL_FALSE, (float*)&glm::translate(translateVec));
	drawHealth(_weapon);

	printShop("[2] Armor:", 16, 88);
	translateVec = glm::vec3(simple(23.5f), simple(88), 0.f);
	glUniformMatrix4fv(glGetUniformLocation(programStatic, "transformation"),
		1, GL_FALSE, (float*)&glm::translate(translateVec));
	drawHealth(_armor);
	std::string src = std::to_string(sources);
	printShop(src, 94, 94);
	printShop("> <", 49, 48);

	std::stringstream stream;
	stream << std::fixed << std::setprecision(1) << acceleration;
	std::string shipSpeed = stream.str();
	printShop("Speed: ", 3, 3);
	printShop(shipSpeed, 7, 3);
}

void drawPlanets() {

	float time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	glm::mat4 translation = glm::translate(glm::vec3(300.0f, 0.0f, -500.0f));
	glm::mat4 rotation = glm::rotate(time, glm::vec3(0.0f, 0.0f, 0.0f));
	glm::mat4 scale = glm::scale(glm::vec3(50.f, 50.f, 50.f));

	//główna planeta
	drawObjectTexture(&sphereModel, translation * scale, texturePlanetAnimated[1]);
	//mniejsze
	rotation = glm::rotate(time / 2.0f, glm::vec3(0.0f, 0.5f, 0.0f));
	scale = glm::scale(glm::vec3(20.f, 20.f, 20.f));
	drawObjectTexture(&sphereModel, translation * glm::rotate(time / 2.0f, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::translate(glm::vec3(100.0f, 0.0f, 0.0f)) * glm::scale(glm::vec3(15.f, 15.f, 15.f)), texturePlanetAnimated[0]);
	drawObjectTexture(&sphereModel, translation * rotation * glm::translate(glm::vec3(0.0f, 0.0f, 100.0f)) * scale, texturePlanetAnimated[0]);
	//najmniejsza
	rotation = glm::rotate(time / 3.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	scale = glm::scale(glm::vec3(10.f, 10.f, 10.f));
	drawObjectTexture(&sphereModel, translation * glm::rotate(time / 2.0f, glm::vec3(0.0f, 0.5f, 0.0f)) * rotation * glm::translate(glm::vec3(100.0f, 30.0f, 0.0f)) * scale, texturePlanet);

}

void setSpotLight() {
	glUseProgram(programTexture);
	glUniform3fv(glGetUniformLocation(programTexture, "spotLight.position"), 1, (float*)&cameraPos);
	glUniform3fv(glGetUniformLocation(programTexture, "spotLight.direction"), 1, (float*)&cameraDir);
	glUniform3fv(glGetUniformLocation(programTexture, "spotLight.color"), 1, (float*)&glm::vec3(1.0f, 1.0f, 0.75f));
	glUniform1f(glGetUniformLocation(programTexture, "spotLight.constant"), 1.0f);
	glUniform1f(glGetUniformLocation(programTexture, "spotLight.linear"), 0.009f);
	glUniform1f(glGetUniformLocation(programTexture, "spotLight.quadratic"), 0.00004f);
	glUniform1f(glGetUniformLocation(programTexture, "spotLight.cutOff"), glm::cos(glm::radians(20.0f)));
	glUniform1f(glGetUniformLocation(programTexture, "spotLight.outerCutOff"), glm::cos(glm::radians(28.0f)));
	glUniform3fv(glGetUniformLocation(programTexture, "viewPos"), 1, (float*)&cameraPos);
	glUseProgram(0);
}


void drawAsteroids() {

	for (int i = 0; i < renderablesAsteroids.size(); i++) {
		PxVec3 currentAstPos = asteroidsBodies[i]->getGlobalPose().p;
		glm::vec3 currentAstGlmPos = glm::vec3(currentAstPos.x, currentAstPos.y, currentAstPos.z);
		glm::mat4 transformation = glm::translate(currentAstGlmPos);
		PxVec3 shipPosition = shipBody->getGlobalPose().p;
		glm::vec3 ballRandVector = glm::ballRand(20.f);
		glm::vec3 shipPosVector = glm::vec3(shipPosition.x, shipPosition.y, shipPosition.z);
		glm::vec3 diff = shipPosVector - currentAstGlmPos;
		if (diff.x > 50 || diff.y > 50 || diff.z > 50) {

			glm::vec3 astVelocityVector = shipPosVector + ballRandVector;
			glm::vec3 transformationVector = glm::ballRand(RADIUS) + glm::vec3(10.f) + shipPosVector;
			glm::mat4 transformation = glm::translate(transformationVector);
			asteroidsBodies[i]->setGlobalPose(PxTransform(transformationVector.x, transformationVector.y, transformationVector.z));
			currentAstPos = PxVec3(transformationVector.x, transformationVector.y, transformationVector.z);
			asteroidsBodies[i]->setLinearVelocity(PxVec3(PxVec3(astVelocityVector.x, astVelocityVector.y, astVelocityVector.z) - currentAstPos) * asteroidAcceleration);
			drawObjectTextureFromContext(renderablesAsteroids[i]->context, transformation, renderablesAsteroids[i]->textureId);
		}
		else {
			drawObjectTextureFromContext(renderablesAsteroids[i]->context, transformation, renderablesAsteroids[i]->textureId);
		}
	}
}

void drawRay(Core::RayContext& context) {
	glUseProgram(programRed);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programRed, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
	context.render();
	glUseProgram(0);
}

//------------------------------------------------------------------
// RENDER

void updateTransforms()
{
	// Here we retrieve the current transforms of the objects from the physical simulation.
	auto actorFlags = PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC;
	PxU32 nbActors = pxScene.scene->getNbActors(actorFlags);
	if (nbActors)
	{
		std::vector<PxRigidActor*> actors(nbActors);
		pxScene.scene->getActors(actorFlags, (PxActor**)&actors[0], nbActors);

		for (auto actor : actors)
		{
			// We use the userData of the objects to set up the model matrices
			// of proper renderables.
			if (!actor->userData) continue;
			Renderable* renderable = (Renderable*)actor->userData;

			// get world matrix of the object (actor)
			PxMat44 transform = actor->getGlobalPose();
			auto& c0 = transform.column0;
			auto& c1 = transform.column1;
			auto& c2 = transform.column2;
			auto& c3 = transform.column3;

			// set up the model matrix used for the rendering
			renderable->modelMatrix = glm::mat4(
				c0.x, c0.y, c0.z, c0.w,
				c1.x, c1.y, c1.z, c1.w,
				c2.x, c2.y, c2.z, c2.w,
				c3.x, c3.y, c3.z, c3.w);
		}
	}
}

void renderScene()
{
	double time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
	static double prevTime = time;
	double dtime = time - prevTime;
	prevTime = time;

	// Update physics
	if (dtime < 1.f) {
		physicsTimeToProcess += dtime;
		while (physicsTimeToProcess > 0) {
			// here we perform the physics simulation step
			pxScene.step(physicsStepTime);
			physicsTimeToProcess -= physicsStepTime;
		}
	}

	changeFlightDir();
	PxVec3 transform = shipBody->getGlobalPose().p;
	cameraPos = glm::vec3(transform.x, transform.y, transform.z);

	// Aktualizacja macierzy widoku i rzutowania
	cameraMatrix = createCameraMatrix();
	perspectiveMatrix = Core::createPerspectiveMatrix(0.1f, 1000.0f, float(windowWidth / windowHeight));

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.1f, 0.3f, 1.0f);

	setSpotLight();

	Skybox::drawSkybox(programSkybox, cameraMatrix, perspectiveMatrix, cubemapTexture);

	glm::mat4 shipInitialTransformation = glm::translate(glm::vec3(0, -0.25f, 0)) * glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.25f));
	glm::mat4 shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::mat4_cast(glm::inverse(rotation)) * shipInitialTransformation;

	if (!explode) {
		drawObjectTextureFromContext(renderables[0]->context, shipModelMatrix * glm::scale(glm::vec3(0.075f)), renderables[0]->textureId);
		expl_time = 0.0;
	}
	else if (expl_time <= 3.5) {
		drawObjectExplode(renderables[0]->context, shipModelMatrix * glm::scale(glm::vec3(0.075f)), renderables[0]->textureId);
		particleEmitter_ShipExplode->update(0.01f, shipModelMatrix, cameraMatrix, perspectiveMatrix);
		particleEmitter_ShipExplode->draw();
	}

	// gems
	for (int i = 1; i < renderables.size(); i++) {

		glm::mat4 transformation = renderables[i]->modelMatrix;
		drawObjectTextureFromContext(renderables[i]->context, transformation, renderables[i]->textureId);

		/*glm::vec3 position = cameraPos;
		PxVec3 gemPos = gemBody->getGlobalPose().p;
		float distance = glm::distance(glm::vec3(position.x, position.y, position.z), glm::vec3(gemPos.x, gemPos.y, gemPos.z));*/

		/*if (distance < 7.f) {
			
			if (i == renderables.size() - 1) {

				renderables.erase(renderables.begin() + i);
			}
			else {

				renderables.erase(renderables.begin() + i, renderables.begin() + i + 1);
			}
			
			cout << "- gem at: ( " << position.x << ", " << position.y << ", " << position.z << ")" << endl;
			addCash();
		}*/
	}

	//asteroids
	drawAsteroids();
	if (isKaboom) {
		asteroidGoesKaboom(kaboomAstPos);
		//Sleep(200);
		//isKaboom = false;
	}

	//planets
	drawPlanets();

	//UI
	glUseProgram(programStatic);
	drawStaticScene(amountHp, amountEngines, amountArmor, amountSources);
	glUseProgram(0);

	//Engine particles
	if (engineON) {

		particleEmitter_LeftEngine->update(0.01f, shipModelMatrix * glm::translate(engineOffset) * engineRotation, cameraMatrix, perspectiveMatrix);
		particleEmitter_LeftEngine->draw();

		particleEmitter_RightEngine->update(0.01f, shipModelMatrix * glm::translate(engineOffset - glm::vec3(engineOffset.x * 2, 0, 0)) * engineRotation, cameraMatrix, perspectiveMatrix);
		particleEmitter_RightEngine->draw();
	}

	if (isShooting && canShoot) {

		updateRay();
		drawRay(rayContext);
	}

	updateTransforms();
	glutSwapBuffers();
}

//----------------------------------------------
// INIT FUNCTIONS

void initPhysicsScene()
{
	asteroidMaterial = pxScene.physics->createMaterial(2, 2, 2);

	for (int i = 0; i < ASTEROIDS_NUMBER; i++) {

		glm::vec3 initPos = astPositions[i];
		PxRigidDynamic* asteroidBody = pxScene.physics->createRigidDynamic(PxTransform(initPos.x, initPos.y, initPos.z));
		PxShape* asteroidShape = pxScene.physics->createShape(PxSphereGeometry(2.f), *asteroidMaterial);
		asteroidBody->attachShape(*asteroidShape);
		asteroidShape->release();
		asteroidBody->setName("asteroid");
		asteroidBody->userData = renderablesAsteroids[i];
		pxScene.scene->addActor(*asteroidBody);
		asteroidsBodies.push_back(asteroidBody);
	}

	shipMaterial = pxScene.physics->createMaterial(5, 5, 5);
	shipBody = pxScene.physics->createRigidDynamic(PxTransform(1, 1, 0));
	PxShape* shipShape = pxScene.physics->createShape(PxSphereGeometry(.1f), *shipMaterial);
	shipBody->attachShape(*shipShape);
	shipShape->release();
	shipBody->setName("ship");
	shipBody->userData = renderables[0];
	pxScene.scene->addActor(*shipBody);
}

void initAsteroids() {

	// load asteroids textures
	for (const auto& file : fs::directory_iterator("textures/asteroids")) 
	{
		asteroidTextures.push_back(Core::LoadTexture(file.path().string().c_str()));
		cout << "Loaded:" << file.path().string().c_str() << endl;
	}

	// load asteroids contexts
	for (const auto& file : fs::directory_iterator("models/asteroids"))
	{
		obj::Model model = obj::loadModelFromFile(file.path().string().c_str());
		Core::RenderContext context;
		context.initFromOBJ(model);
		asteroidContexts.push_back(context);
	}

	for(int i=0; i< ASTEROIDS_NUMBER; i++){
		astPositions.push_back(glm::ballRand(RADIUS));
	}
}

void initAsteroidsRenderables() {

	while (ACTUAL_ASTEROIDS_NUMBER < ASTEROIDS_NUMBER) {

		int textureIndex = rand() % asteroidTextures.size();
		int contextIndex = rand() % asteroidContexts.size();
		GLuint texture = asteroidTextures.at(textureIndex);

		Renderable* asteroid = new Renderable();
		asteroid->context = &asteroidContexts.at(contextIndex);
		asteroid->textureId = texture;
		renderablesAsteroids.emplace_back(asteroid);

		ACTUAL_ASTEROIDS_NUMBER++;
	}
}

void initAsteroidsVelocity() {

	for (int i = 0; i < ASTEROIDS_NUMBER; i++) {

		PxVec3 currentAstPos = asteroidsBodies[i]->getGlobalPose().p;
		PxVec3 shipPosition = shipBody->getGlobalPose().p;
		glm::vec3 ballRandVector = glm::ballRand(20.f);
		glm::vec3 shipPosVector = glm::vec3(shipPosition.x, shipPosition.y, shipPosition.z);
		glm::vec3 astVelocityVector = shipPosVector + ballRandVector;
		asteroidsBodies[i]->setLinearVelocity(PxVec3(PxVec3(astVelocityVector.x, astVelocityVector.y, astVelocityVector.z) - currentAstPos) * .1f);
	}
}

void initRenderables()
{

	// load models
	shipModel = obj::loadModelFromFile("models/spaceship_cruiser.obj");
	shipContext.initFromOBJ(shipModel);
	gemModel = obj::loadModelFromFile("models/gem.obj");
	gemContext.initFromOBJ(gemModel);

	// load textures
	textureShip = Core::LoadTexture("textures/ship/cruiser01_diffuse.png");
	textureGem = Core::LoadTexture("textures/gem.png");

	// ship
	Renderable* ship = new Renderable();
	ship->context = &shipContext;
	ship->textureId = textureShip;
	renderables.emplace_back(ship);

	// asteroids
	initAsteroidsRenderables();
}

void initStatic() {

	amountHp = 10;
	amountEngines = 1;
	amountArmor = 1;
	amountSources = 30;
}

void init()
{
	srand(time(0));
	glEnable(GL_DEPTH_TEST);
	programColor = shaderLoader.CreateProgram("shaders/shader_color.vert", "shaders/shader_color.frag");
	programTexture = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
	programSkybox = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");
	programStatic = shaderLoader.CreateProgram("shaders/shader_hp.vert", "shaders/shader_hp.frag");
	programExplode = shaderLoader.CreateProgram("shaders/shader_explode.vert", "shaders/shader_explode.frag", "shaders/shader_explode.geom");
	programEngineParticle = shaderLoader.CreateProgram("shaders/part.vert", "shaders/part.frag");
	programTextureParticle = shaderLoader.CreateProgram("shaders/part_tex.vert", "shaders/part_tex.frag");
	textureShip = Core::LoadTexture("textures/ship/cruiser01_diffuse.png");
	textureShipNormal = Core::LoadTexture("textures/ship/cruiser01_secular.png");
	irrklang::ISound* snd = SoundEngine->play2D("dependencies/irrklang/media/theme.mp3", true, false, true);
	snd->setVolume(0.009f); /*komentuje tylko żeby sobie posłuchać głosniej*/
	//snd->setVolume(0.04f);
	firstMouse = true;
	sphereModel = obj::loadModelFromFile("models/sphere.obj");
	texturePlanet = Core::LoadTexture("textures/asteroids/unnamed.png");
	texturePlanetAnimated.push_back(Core::LoadTexture("textures/planets/jupiter.png"));
	texturePlanetAnimated.push_back(Core::LoadTexture("textures/planets/venus.png"));
	cubemapTexture = Skybox::loadCubemap(faces);
	initAsteroids();
	initRenderables();
	initPhysicsScene();
	initAsteroidsVelocity();
	initStatic();
	engineON = false;
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	
	explosionTexture = Core::LoadTexture("textures/particles/explosion.png");
	programRed = shaderLoader.CreateProgram("shaders/shader_red.vert", "shaders/shader_red.frag");
	Core::initRay(rayContext);
	updateRay();
	glViewport(0, 0, windowWidth, windowHeight);
}

//------------------------------------------------------------------
// MOUSE AND KEYBOARD

void mouse(int x, int y)
{
	float fx = float(x);
	float fy = float(y);

	if (firstMouse) {
		lastX = fx;
		lastY = fy;
		firstMouse = false;
	}

	float xoffset = fx - lastX;
	float yoffset = fy - lastY;
	lastX = fx;
	lastY = fy;
	newMouseX = xoffset;
	newMouseY = yoffset;
}

void click_mouse(int button, int state, int x, int y) {

	if ((GLUT_LEFT_BUTTON == button && state == GLUT_DOWN)) {

		isShooting = true;

		updateRay();

		//here raycast should be done
		PxRaycastBuffer hit;
		pxScene.scene->raycast(vec3ToPxVec(ray[0]), vec3ToPxVec(ray[1]), 1000, hit);

		//check if there is a hit
		if (hit.hasAnyHits() && canShoot) {
			PxRaycastHit block = hit.block;
			//check if it is rigid dynamic
			if (block.actor->getType() == PxActorType::eRIGID_DYNAMIC) {
				PxRigidDynamic* actor = (PxRigidDynamic*)block.actor;
				Renderable* actorRenderable = (Renderable*)actor->userData;
				string actorName = "";
				if (actorRenderable->context == &shipContext) actorName = "ship";
				else if (actorRenderable->context == &gemContext) {
					actorName = "gem";
				}
				//else if (actorRenderable->context == &asteroidContext)
				else
				{
					actorName = "asteroid";
					glm::vec3 positionVec = glm::vec3(actor->getGlobalPose().p.x, actor->getGlobalPose().p.y, actor->getGlobalPose().p.z);
					glm::mat4 currentAstPos = glm::translate(positionVec);
					//Do sth with hit asteroid
					actor->setLinearVelocity(vec3ToPxVec(cameraDir * 500));
					particleEmitter_AstExplode = new ParticleEmitterTex(&programTextureParticle, 2500, 0.7f, explosionTexture);
					kaboomAstPos = currentAstPos;
					isKaboom = true;
					asteroidsDestroyed++;

					generateGem(positionVec.x, positionVec.y, positionVec.z);

					if (asteroidsDestroyed == 5)	asteroidAcceleration = 0.1f;
					else if (asteroidsDestroyed == 10) asteroidAcceleration = 0.2f;
					else if (asteroidsDestroyed == 20) asteroidAcceleration = 0.3f;
					else if (asteroidsDestroyed == 50) asteroidAcceleration = 0.5f;
				}
			}
		}
		else {
			std::cout << "no hit\n";
		}
	}
	else if ((GLUT_LEFT_BUTTON == button && state == GLUT_UP)) {
		isShooting = false;
	}
}

void keyboard(unsigned char key, int x, int y)
{

	float angleSpeed = .1f;
	float moveSpeed = 1.0f;
	switch (key)
	{
	case 'z': rotation = glm::angleAxis(angleSpeed, glm::vec3(0, 0, 1)) * rotation; break;
	case 'x': rotation = glm::angleAxis(angleSpeed, glm::vec3(0, 0, -1)) * rotation; break;
	case 'w': {
		//cameraPos += cameraDir * moveSpeed;
		if (amountHp != 0) {
			speedUp();
			enableEngines();
		}
		break;
	}
	case 's': {
		//cameraPos -= cameraDir * moveSpeed;
		slowDown();
		disableEngines();
		break;
	}
	case 'd': cameraPos += cameraSide * moveSpeed; break;
	case 'a': cameraPos -= cameraSide * moveSpeed; break;
		//case 'e': explode = true; particleEmitter_ShipExplode = new ParticleEmitterTex(&programTextureParticle, 2000, 0.05, explosionTexture); disableEngines(); break;
	case 'r': {
		explode = false;
		canShoot = true;
		amountHp = 4;
		break; }
	case '1': upEngines(); break;
	case '2': upArmor(); break;
	case '3': addCash(); break;
	case 27: glutDestroyWindow(winHandle); break;
	}
}

void shutdown()
{
	shaderLoader.DeleteProgram(programColor);
	shaderLoader.DeleteProgram(programTexture);
	shaderLoader.DeleteProgram(programSkybox);
	shaderLoader.DeleteProgram(programExplode);
	shaderLoader.DeleteProgram(programStatic);
	shaderLoader.DeleteProgram(programTextureParticle);
	shaderLoader.DeleteProgram(programEngineParticle);

	particleEmitter_LeftEngine->~ParticleEmitter();
	particleEmitter_RightEngine->~ParticleEmitter();
	particleEmitter_ShipExplode->~ParticleEmitterTex();
	particleEmitter_AstExplode->~ParticleEmitterTex();
}

void idle()
{
	glutPostRedisplay();
}


int main(int argc, char ** argv)
{
	srand(time(NULL));
	glutInit(&argc, argv);
	glutSetOption(GLUT_MULTISAMPLE, 8);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
	glEnable(GL_MULTISAMPLE);
	glutInitWindowPosition(200, 200);
	glutInitWindowSize(windowWidth, windowHeight);
	winHandle = glutCreateWindow("Space shitter");
	glewInit();
	windowWidth = glutGet(GLUT_SCREEN_WIDTH);
	windowHeight = glutGet(GLUT_SCREEN_HEIGHT);
	glutFullScreen();

	glutSetCursor(GLUT_CURSOR_NONE);
	init();
	glutKeyboardFunc(keyboard);
	glutPassiveMotionFunc(mouse);
	glutMotionFunc(mouse);
	glutMouseFunc(click_mouse);
	glutDisplayFunc(renderScene);
	glutIdleFunc(idle);

	glutMainLoop();

	shutdown();

	return 0;
}