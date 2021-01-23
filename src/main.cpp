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

using namespace irrklang;
#pragma comment(lib, "irrKlang.lib") // chuj wie co to robi

ISoundEngine* SoundEngine = createIrrKlangDevice();

int windowWidth = 600;
int windowHeight = 600;
GLuint programColor;
GLuint programTexture;
GLuint programCubemap;
GLuint programSkybox;
GLuint programStatic;
GLuint programExplode;

Core::RenderContext shipContext;

const float RADIUS = 100.f;
const int ASTEROIDS_NUMBER = 50;

std::vector<GLuint> asteroidTextures;
std::vector<obj::Model> asteroidModels;
std::vector<Asteroid> asteroids;

GLuint textureShip;
GLuint textureShipNormal;

GLuint cubemapTexture;
GLuint skyboxVAO, skyboxVBO;

Core::Shader_Loader shaderLoader;

obj::Model shipModel, sphereModel;


glm::vec3 cameraPos = glm::vec3(0, 0, 0);
glm::vec3 cameraDir; // Wektor "do przodu" kamery
glm::vec3 cameraSide; // Wektor "w bok" kamery
float cameraAngle = 0;

glm::mat4 cameraMatrix, perspectiveMatrix;

glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, -0.9f, -1.0f));

glm::quat rotation = glm::quat(1, 0, 0, 0);

//PhysX init
Physics pxScene(0.f);
// fixed timestep for stable and deterministic simulation
const double physicsStepTime = 1.f / 60.f;
double physicsTimeToProcess = 0;

// physical objects
PxRigidStatic *asteroidBody = nullptr;
PxMaterial *asteroidMaterial = nullptr;
PxRigidDynamic *shipBody = nullptr;
PxMaterial *shipMaterial = nullptr;

// renderable objects (description of a single renderable instance)
struct Renderable {
	Core::RenderContext* context;
	glm::mat4 modelMatrix;
	GLuint textureId;
};
std::vector<Renderable*> renderables;


void initRenderables()
{
	Renderable *ship = new Renderable();
	ship->context = &shipContext;
	ship->textureId = textureShip;
	renderables.emplace_back(ship);

	// load models
	shipModel = obj::loadModelFromFile("models/spaceship_cruiser.obj");
	shipContext.initFromOBJ(shipModel);

	// load textures
	textureShip = Core::LoadTexture("textures/ship/cruiser01_diffuse.png");

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
float expl_speed = 0.05f;

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

void keyboard(unsigned char key, int x, int y)
{
	
	float angleSpeed = .1f;
	float moveSpeed = 1.0f;
	switch(key)
	{	
	case 'z': rotation = glm::angleAxis(angleSpeed, glm::vec3(0, 0, 1))* rotation; break;
	case 'x': rotation = glm::angleAxis(angleSpeed, glm::vec3(0, 0, -1))* rotation; break;
	case 'w': cameraPos += cameraDir * moveSpeed; break;
	case 's': cameraPos -= cameraDir * moveSpeed; break;
	case 'd': cameraPos += cameraSide * moveSpeed; break;
	case 'a': cameraPos -= cameraSide * moveSpeed; break;
	case 'e': explode = true; break;
	case 'r': explode = false; break;
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
	gluOrtho2D(0.0, 600, 0.0, 600);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	glRasterPos2i(0.01*x*windowWidth, 0.01*y*windowHeight);
	std::string s = text;
	void * font = GLUT_BITMAP_9_BY_15;
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
		glVertex2f(0.1f, i);
		glVertex2f(0.1f, i + barHeight);
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
	glm::vec3 translateVec = glm::vec3(simple(92), simple(3), 0.f);
	glUniformMatrix4fv(glGetUniformLocation(programStatic, "transformation"),
		1, GL_FALSE, (float*)&glm::translate(translateVec));

	glm::vec3 color = glm::vec3(255, 0, 0);
	glUniform3f(glGetUniformLocation(programStatic, "objectColor"), color.x, color.y, color.z);
	drawHealth(_hp);
	color = glm::vec3(255, 255, 0);
	glUniform3f(glGetUniformLocation(programStatic, "objectColor"), color.x, color.y, color.z);

	printShop("Weapon:", 1, 88);
	translateVec = glm::vec3(simple(12.5f), simple(88), 0.f);
	glUniformMatrix4fv(glGetUniformLocation(programStatic, "transformation"),
		1, GL_FALSE, (float*)&glm::translate(translateVec));
	drawHealth(_weapon);

	printShop("Armor:", 26, 88);
	translateVec = glm::vec3(simple(36), simple(88), 0.f);
	glUniformMatrix4fv(glGetUniformLocation(programStatic, "transformation"),
		1, GL_FALSE, (float*)&glm::translate(translateVec));
	drawHealth(_armor);
	std::string src = std::to_string(sources);
	printShop(src, 92, 92);
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
	perspectiveMatrix = Core::createPerspectiveMatrix();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.1f, 0.3f, 1.0f);


	glm::mat4 shipInitialTransformation = glm::translate(glm::vec3(0, -0.25f, 0)) * glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.25f));
	glm::mat4 shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::mat4_cast(glm::inverse(rotation)) * shipInitialTransformation;
	

	if (!explode) {
		drawObjectTextureFromContext(renderables[0]->context, shipModelMatrix * glm::scale(glm::vec3(0.075f)), renderables[0]->textureId);
		expl_time = 0.0;
	}
	else if (expl_time <= 2.0){
		drawObjectExplode(renderables[0]->context, shipModelMatrix* glm::scale(glm::vec3(0.075f)), renderables[0]->textureId);
	}	
	//drawObjectTextureFromContext(renderables[0]->context, shipModelMatrix* glm::scale(glm::vec3(0.075f)), renderables[0]->textureId);


	for (Asteroid asteroid : asteroids) {

		drawObjectTexture(&asteroid.Model, asteroid.Coordinates, asteroid.Texture);
	}
	
	Skybox::drawSkybox(programSkybox, cameraMatrix, perspectiveMatrix, cubemapTexture);		
	
	glUseProgram(programStatic);
	drawStaticScene(5,4,3, 30);
	glUseProgram(0);

	glutSwapBuffers();
}

void initAsteroids() {

	// �adowanie textur asteroid
	for (const auto& file : fs::directory_iterator("textures/asteroids/"))
		asteroidTextures.push_back(Core::LoadTexture(file.path().string().c_str()));

	// �adowanie dost�pnych modeli asteroid
	for (const auto& file : fs::directory_iterator("models/asteroids/"))
		asteroidModels.push_back(obj::loadModelFromFile(file.path().string().c_str()));

	// generowanie losowych danych dla asteroid
	for (int i = 0; i < ASTEROIDS_NUMBER; i++) {

		int textureIndex = rand() % asteroidTextures.size();
		int modelIndex = rand() % asteroidModels.size();

		GLuint texture = asteroidTextures.at(textureIndex);
		obj::Model model = asteroidModels.at(modelIndex);

		asteroids.push_back(Asteroid(glm::ballRand(RADIUS), texture, model));
	}
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
	snd->setVolume(0.009f);
	firstMouse = true;
	
	cubemapTexture = Skybox::loadCubemap(faces);
	initAsteroids();
	initRenderables();
	initPhysicsScene();
	//Core::setA(1);
	//std::cout << Core::getA();
}

void shutdown()
{
	shaderLoader.DeleteProgram(programColor);
	shaderLoader.DeleteProgram(programTexture);
	shaderLoader.DeleteProgram(programSkybox);
	shaderLoader.DeleteProgram(programExplode);
	shaderLoader.DeleteProgram(programStatic);
}

void idle()
{
	glutPostRedisplay();
}

void onReshape(int width, int height)
{	
	//Core::setFrustumScale((float)width / height);
	glViewport(0, 0, width, height);
}

int main(int argc, char ** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(200, 200);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("Space shitter");
	glewInit();
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