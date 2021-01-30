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
// obj
ParticleEmitter* particleEmitter_LeftEngine;
ParticleEmitter* particleEmitter_RightEngine;
ParticleEmitterTex* particleEmitter_ShipExplode;
// offset
const glm::vec3 engineOffset = glm::vec3(-0.23f, -0.08f, -0.4f);
const glm::mat4 engineRotation = glm::rotate(glm::radians(90.0f), glm::vec3(1, 0, 0));
// particle program
GLuint programEngineParticle;
// ...
GLuint explosionTexture;


using namespace irrklang;
//#pragma comment(lib, "irrKlang.lib") // chuj wie co to robi // już nic hehe bo skomentowane

ISoundEngine* SoundEngine = createIrrKlangDevice();


int windowWidth = 600;
int windowHeight = 600;
GLuint programColor;
GLuint programTexture;
GLuint programCubemap;
GLuint programSkybox;
GLuint programStatic;
GLuint programExplode;
GLuint programTextureParticle;

Core::RenderContext shipContext;
Core::RenderContext gemContext;

const float RADIUS = 100.f;
const int ASTEROIDS_NUMBER = 50;

std::vector<GLuint> asteroidTextures;
std::vector<obj::Model> asteroidModels;
std::vector<Asteroid> asteroids;
std::vector<GLuint> texturePlanetAnimated;

GLuint textureShip;
GLuint textureShipNormal;
GLuint texturePlanet;
GLuint textureGem;

GLuint cubemapTexture;
GLuint skyboxVAO, skyboxVBO;

Core::Shader_Loader shaderLoader;

obj::Model shipModel, sphereModel, gemModel;


glm::vec3 cameraPos = glm::vec3(0, 0, 0);
glm::vec3 cameraDir; // Wektor "do przodu" kamery
glm::vec3 cameraSide; // Wektor "w bok" kamery
float cameraAngle = 0;

glm::mat4 cameraMatrix, perspectiveMatrix;

glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, -0.9f, -1.0f));

glm::quat rotation = glm::quat(1, 0, 0, 0);


int amountHp, amountArmor, amountWeapon, amountSources;

// physical objects
PxRigidStatic *asteroidBody = nullptr;
PxMaterial *asteroidMaterial = nullptr;
PxRigidDynamic* shipBody = nullptr;
PxMaterial* shipMaterial = nullptr;
PxRigidDynamic* gemBody = nullptr;
PxMaterial* gemMaterial = nullptr;

// renderable objects (description of a single renderable instance)
struct Renderable {
	Core::RenderContext* context;
	glm::mat4 modelMatrix;
	GLuint textureId;
};
std::vector<Renderable*> renderables;

//------------------------------------------------------------------
// contact pairs filtering function
static PxFilterFlags simulationFilterShader(PxFilterObjectAttributes attributes0,
	PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	pairFlags =
		PxPairFlag::eCONTACT_DEFAULT | // default contact processing
		PxPairFlag::eNOTIFY_CONTACT_POINTS | // contact points will be available in onContact callback
		PxPairFlag::eNOTIFY_TOUCH_FOUND; // onContact callback will be called for this pair
		//PxPairFlag::eNOTIFY_TOUCH_PERSISTS;

	return physx::PxFilterFlag::eDEFAULT;
}

//------------------------------------------------------------------
// simulation events processor
class SimulationEventCallback : public PxSimulationEventCallback
{
public:
	void onContact(const PxContactPairHeader& pairHeader,
		const PxContactPair* pairs, PxU32 nbPairs)
	{
		for (PxU32 i = 0; i < nbPairs; i++)
		{
			const PxContactPair& cp = pairs[i];

			PxU32 bufferSize = cp.contactCount;
			std::cout << "Pair: " << i << "; number of contact points: " << bufferSize << std::endl;

			PxContactPairPoint* buffer = new PxContactPairPoint[bufferSize];
			cp.extractContacts(buffer, bufferSize);

			for (PxU32 i = 0; i < bufferSize; i++)
			{
				std::cout << "(x, y, z) = (" << buffer[i].position.x << ", " << buffer[i].position.y << ", " << buffer[i].position.z << ")" << std::endl;
			}
		}
		
	}

	// The functions below are not used in this exercise.
	// However, they need to be defined for the class to compile.
	virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) {}
	virtual void onWake(PxActor** actors, PxU32 count) {}
	virtual void onSleep(PxActor** actors, PxU32 count) {}
	virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) {}
	virtual void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) {}
};
//----------------------------------------------

//PhysX init
SimulationEventCallback simulationEventCallback;
Physics pxScene(0.f, simulationFilterShader, &simulationEventCallback);

// fixed timestep for stable and deterministic simulation
const double physicsStepTime = 1.f / 60.f;
double physicsTimeToProcess = 0;

//----------------------------------------------


void enableEngines();
void disableEngines();


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

	// gem
	Renderable* gem = new Renderable();
	gem->context = &gemContext;
	gem->textureId = textureGem;
	renderables.emplace_back(gem);
}

void generateGem(float x, float y, float z) {

	gemMaterial = pxScene.physics->createMaterial(1, 1, 1);
	PxRigidDynamic* gemBody = pxScene.physics->createRigidDynamic(PxTransform(x, y, z));
	PxShape* gemShape = pxScene.physics->createShape(PxSphereGeometry(2.f), *gemMaterial);
	gemBody->attachShape(*gemShape);
	gemShape->release();

	gemBody->userData = renderables[1];
	pxScene.scene->addActor(*gemBody);
}


void initPhysicsScene()
{
	shipMaterial = pxScene.physics->createMaterial(1, 1, 1);
	PxRigidDynamic* shipBody = pxScene.physics->createRigidDynamic(PxTransform(cameraPos.x, cameraPos.y, cameraPos.z));
	
	PxShape* shipShape = pxScene.physics->createShape(PxSphereGeometry(2.f), *shipMaterial);
	shipBody->attachShape(*shipShape);
	shipShape->release();
	shipBody->userData = renderables[0];
	pxScene.scene->addActor(*shipBody);

	generateGem(1, 2, 2);
}

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
			Renderable *renderable = (Renderable*)actor->userData;

			// get world matrix of the object (actor)
			PxMat44 transform = actor->getGlobalPose();
			auto &c0 = transform.column0;
			auto &c1 = transform.column1;
			auto &c2 = transform.column2;
			auto &c3 = transform.column3;

			// set up the model matrix used for the rendering
			renderable->modelMatrix = glm::mat4(
				c0.x, c0.y, c0.z, c0.w,
				c1.x, c1.y, c1.z, c1.w,
				c2.x, c2.y, c2.z, c2.w,
				c3.x, c3.y, c3.z, c3.w);
		}
	}
}

bool firstMouse;
float newMouseX = 0;
float newMouseY = 0;
float lastX = 0;
float lastY = 0;
float mouseSpeed = 0.5f;

bool explode = false;
float expl_time = 0.0f;
float expl_speed = 0.03f;

glm::quat rotationZ = glm::angleAxis(glm::radians(0.f), glm::vec3(0, 0, 1));


std::vector<std::string> faces
{
	"textures/skybox/space.jpg",
	"textures/skybox/space.jpg",
	"textures/skybox/space.jpg",
	"textures/skybox/space.jpg",
	"textures/skybox/space.jpg",
	"textures/skybox/space.jpg",	
};

void onHit() {
	amountHp++;
}

void sourceGrab() {
	amountSources++;
}

void upArmor() {
	if (amountArmor < 4) {
		int currentBalance = amountSources;
		switch (amountArmor) {
		case 1:
			if (currentBalance >= 10) {
				amountArmor++;
				currentBalance -= 10;
				//setWeaponStrength(weaponStrength++)
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

void upWeapon() {
	if (amountWeapon < 4) {
		int currentBalance = amountSources;
		switch (amountWeapon) {
		case 1:
			if (currentBalance >= 10) {
				amountWeapon++;
				currentBalance -= 10;
			};
			break;
		case 2:
			if (currentBalance >= 20) {
				amountWeapon++;
				currentBalance -= 20;
			};
			break;
		case 3:
			if (currentBalance >= 30) {
				amountWeapon++;
				currentBalance -= 30;
			};
			break;
		}
		amountSources = currentBalance;
	}
}
void addCash() {
	//just to test
	amountSources++;
}
bool engineON;

void keyboard(unsigned char key, int x, int y)
{
	
	float angleSpeed = .1f;
	float moveSpeed = 1.0f;
	switch(key)
	{	
	case 'z': rotation = glm::angleAxis(angleSpeed, glm::vec3(0, 0, 1))* rotation; break;
	case 'x': rotation = glm::angleAxis(angleSpeed, glm::vec3(0, 0, -1))* rotation; break;
	case 'w': cameraPos += cameraDir * moveSpeed; enableEngines(); break;
	case 's': cameraPos -= cameraDir * moveSpeed; disableEngines(); break;
	case 'd': cameraPos += cameraSide * moveSpeed; break;
	case 'a': cameraPos -= cameraSide * moveSpeed; break;
	case 'e': explode = true; particleEmitter_ShipExplode = new ParticleEmitterTex(&programTextureParticle, 2000, 0.05, explosionTexture); disableEngines(); break;
	case 'r': explode = false; break;
	case '1': upWeapon(); break;
	case '2': upArmor(); break;
	case '3': addCash(); break;
	}
}

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


glm::mat4 createCameraMatrix()
{
	glm::quat rotationChange;
	rotationChange = glm::angleAxis(float(newMouseY)*.02f, glm::vec3(1.f, 0.f, 0.f))*glm::angleAxis(float(newMouseX)*.02f, glm::vec3(0.f, 1.f, 0.f));
	newMouseX = 0;
	newMouseY = 0;
	rotation = rotationChange * rotation;
	rotation = normalize(rotation);
	cameraDir = glm::inverse(rotation) * glm::vec3(0, 0, -1.f);
	cameraSide = glm::inverse(rotation) * glm::vec3(1.f, 0, 0);

	return Core::createViewMatrixQuat(cameraPos, rotation);
}

void drawObjectColor(obj::Model * model, glm::mat4 modelMatrix, glm::vec3 color)
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

void drawObjectTexture(obj::Model * model, glm::mat4 modelMatrix, GLuint textureId)
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

void drawObjectTextureFromContext(Core::RenderContext * context, glm::mat4 modelMatrix, GLuint textureId)
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
	glWindowPos2i(0.01*x*windowWidth, 0.01*y*windowHeight);
	std::string s = text;
	void * font = GLUT_BITMAP_TIMES_ROMAN_24;
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
	return (-1.f+0.02f*n);
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

	printShop("[1] Weapon:", 1, 88);
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
	glUniform1f(glGetUniformLocation(programTexture, "spotLight.linear"), 0.009);
	glUniform1f(glGetUniformLocation(programTexture, "spotLight.quadratic"), 0.00004);
	glUniform1f(glGetUniformLocation(programTexture, "spotLight.cutOff"), glm::cos(glm::radians(20.0f)));
	glUniform1f(glGetUniformLocation(programTexture, "spotLight.outerCutOff"), glm::cos(glm::radians(28.0f)));
	glUniform3fv(glGetUniformLocation(programTexture, "viewPos"), 1, (float*)&cameraPos);
	glUseProgram(0);
}

void enableEngines() {
	if (!engineON) {
		particleEmitter_LeftEngine = new ParticleEmitter(&programEngineParticle, 5000, 0.0030);
		particleEmitter_RightEngine = new ParticleEmitter(&programEngineParticle, 5000, 0.0030);
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
	
	// Aktualizacja macierzy widoku i rzutowania
	cameraMatrix = createCameraMatrix();
	perspectiveMatrix = Core::createPerspectiveMatrix(0.1f, 1000.0f, float(windowWidth/windowHeight));



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
	else if (expl_time <= 3.5){
		drawObjectExplode(renderables[0]->context, shipModelMatrix* glm::scale(glm::vec3(0.075f)), renderables[0]->textureId);
		particleEmitter_ShipExplode->update(0.01f, shipModelMatrix, cameraMatrix, perspectiveMatrix);
		particleEmitter_ShipExplode->draw();
	}	

	for (Asteroid asteroid : asteroids) {

		drawObjectTexture(&asteroid.Model, asteroid.Coordinates, asteroid.Texture);
	}
	
	drawPlanets();
	
	glUseProgram(programStatic);
	drawStaticScene(amountHp, amountWeapon, amountArmor, amountSources);
	glUseProgram(0);

	//Engine particles
	if (engineON) {
		particleEmitter_LeftEngine->update(0.01f, shipModelMatrix * glm::translate(engineOffset) * engineRotation, cameraMatrix, perspectiveMatrix);
		particleEmitter_LeftEngine->draw();

		particleEmitter_RightEngine->update(0.01f, shipModelMatrix * glm::translate(engineOffset - glm::vec3(engineOffset.x * 2, 0, 0)) * engineRotation, cameraMatrix, perspectiveMatrix);
		particleEmitter_RightEngine->draw();
	}

	updateTransforms();

	//drawObjectTextureFromContext(renderables[0]->context, renderables[0]->modelMatrix, renderables[0]->textureId);
	drawObjectTextureFromContext(renderables[1]->context, renderables[1]->modelMatrix, renderables[1]->textureId);
	
	glutSwapBuffers();
}

void initAsteroids() {

	// load asteroids textures
	for (const auto& file : fs::directory_iterator("textures/asteroids/"))
		asteroidTextures.push_back(Core::LoadTexture(file.path().string().c_str()));

	// load asteroids models
	for (const auto& file : fs::directory_iterator("models/asteroids/"))
		asteroidModels.push_back(obj::loadModelFromFile(file.path().string().c_str()));

	// generate random asteroids
	for (int i = 0; i < ASTEROIDS_NUMBER; i++) {

		int textureIndex = rand() % asteroidTextures.size();
		int modelIndex = rand() % asteroidModels.size();

		GLuint texture = asteroidTextures.at(textureIndex);
		obj::Model model = asteroidModels.at(modelIndex);

		asteroids.push_back(Asteroid(glm::ballRand(RADIUS), texture, model));
	}
}

void initStatic() {
	amountHp = 5;
	amountWeapon = 1;
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
	textureShip = Core::LoadTexture("textures/ship/cruiser01_diffuse.png");
	textureShipNormal = Core::LoadTexture("textures/ship/cruiser01_secular.png");
	irrklang::ISound* snd = SoundEngine->play2D("dependencies/irrklang/media/theme.mp3", true, false, true);
	//snd->setVolume(0.009f); komentuje tylko żeby sobie posłuchać głosniej
	snd->setVolume(0.04f);
	firstMouse = true;
	sphereModel = obj::loadModelFromFile("models/sphere.obj");
	texturePlanet = Core::LoadTexture("textures/asteroids/unnamed.png");
	texturePlanetAnimated.push_back(Core::LoadTexture("textures/planets/jupiter.png"));
	texturePlanetAnimated.push_back(Core::LoadTexture("textures/planets/venus.png"));
	cubemapTexture = Skybox::loadCubemap(faces);
	initAsteroids();
	initRenderables();
	initPhysicsScene();
	initStatic();
	engineON = false;
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	programEngineParticle = shaderLoader.CreateProgram("shaders/part.vert", "shaders/part.frag");
	programTextureParticle = shaderLoader.CreateProgram("shaders/part_tex.vert", "shaders/part_tex.frag");

	explosionTexture = Core::LoadTexture("textures/particles/explosion.png");
	
	
	glViewport(0, 0, windowWidth, windowHeight);
}

void shutdown()
{
	shaderLoader.DeleteProgram(programColor);
	shaderLoader.DeleteProgram(programTexture);
	shaderLoader.DeleteProgram(programSkybox);
	shaderLoader.DeleteProgram(programExplode);
	shaderLoader.DeleteProgram(programStatic);

	particleEmitter_LeftEngine->~ParticleEmitter();
	particleEmitter_RightEngine->~ParticleEmitter();
	particleEmitter_ShipExplode->~ParticleEmitterTex();
}

void idle()
{
	glutPostRedisplay();
}


int main(int argc, char ** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(200, 200);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("Space shitter");
	glewInit();
	windowWidth = glutGet(GLUT_SCREEN_WIDTH);
	windowHeight = glutGet(GLUT_SCREEN_HEIGHT);
	glutFullScreen();

	glutSetCursor(GLUT_CURSOR_NONE);
	init();
	glutKeyboardFunc(keyboard);
	glutPassiveMotionFunc(mouse);
	glutDisplayFunc(renderScene);
	glutIdleFunc(idle);

	glutMainLoop();

	shutdown();

	return 0;
}