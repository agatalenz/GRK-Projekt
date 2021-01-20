#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <vector>

#include "Skybox.h"
#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"
#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "Asteroid.h"

GLuint programColor;
GLuint programTexture;
GLuint programCubemap;
GLuint programSkybox;
GLuint programStatic;
GLuint programExplode;

const float RADIUS = 100.f;
const int ASTEROIDS_NUMBER = 60;

std::vector<GLuint> asteroidTextures;
std::vector<obj::Model> asteroidModels;
std::vector<Asteroid> asteroids;

GLuint textureShip;
GLuint textureShipNormal;

GLuint cubemapTexture;
GLuint skyboxVAO, skyboxVBO;

Core::Shader_Loader shaderLoader;

obj::Model shipModel;
obj::Model sphereModel;


glm::vec3 cameraPos = glm::vec3(0, 0, 0);
glm::vec3 cameraDir; // Wektor "do przodu" kamery
glm::vec3 cameraSide; // Wektor "w bok" kamery
float cameraAngle = 0;

glm::mat4 cameraMatrix, perspectiveMatrix;

glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, -0.9f, -1.0f));

glm::quat rotation = glm::quat(1, 0, 0, 0);


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

void drawHealth4(float health) {
	const int numDiv = 15;
	const float sep = 0.02f;
	const float barHeight = 0.5f / (float)numDiv;
	glBegin(GL_QUADS);
	glColor3f(1, 0, 0);
	for (float i = 0; i < health; i += (sep + barHeight)) {
		glVertex2f(0, i);
		glVertex2f(0.1f, i);
		glVertex2f(0.1f, i + barHeight);
		glVertex2f(0, i + barHeight);
	}
	glEnd();
}

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
	float t = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
	lightDir = glm::vec3(lightDir.x, lightDir.y, lightDir.z);
	glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
	Core::SetActiveTexture(textureId, "textureSampler", program, 0);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	Core::DrawModel(model);

	glUseProgram(0);
}

void drawObjectExplode(obj::Model* model, glm::mat4 modelMatrix, GLuint textureId)
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

	Core::DrawModel(model);

	glUseProgram(0);
}

void renderScene()
{	
	// Aktualizacja macierzy widoku i rzutowania
	cameraMatrix = createCameraMatrix();
	perspectiveMatrix = Core::createPerspectiveMatrix();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.1f, 0.3f, 1.0f);


	glm::mat4 shipInitialTransformation = glm::translate(glm::vec3(0, -0.25f, 0)) * glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.25f));
	glm::mat4 shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::mat4_cast(glm::inverse(rotation)) * shipInitialTransformation;
	
	

	if (!explode) {
		drawObjectExplode(&shipModel, shipModelMatrix* glm::scale(glm::vec3(0.075f)), textureShip);
		expl_time = 0.0;
	}
	else if (expl_time <= 2.0){
		drawObjectExplode(&shipModel, shipModelMatrix* glm::scale(glm::vec3(0.075f)), textureShip);
	}	


	for (Asteroid asteroid : asteroids) {

		drawObjectTexture(&asteroid.Model, asteroid.Coordinates, asteroid.Texture);
	}
	
	Skybox::drawSkybox(programSkybox, cameraMatrix, perspectiveMatrix, cubemapTexture);		
	
	glUseProgram(programStatic);
	glm::mat4 translation = glm::translate(glm::vec3(0.95f - 0.1f, -0.95f, 0.f));
	glUniformMatrix4fv(glGetUniformLocation(programStatic, "transformation"),
		1, GL_FALSE, (float*)&translation);
	drawHealth4(0.3f);
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
	sphereModel = obj::loadModelFromFile("models/sphere.obj");
	shipModel = obj::loadModelFromFile("models/spaceship_cruiser.obj");
	textureShip = Core::LoadTexture("textures/ship/cruiser01_diffuse.png");
	textureShipNormal = Core::LoadTexture("textures/ship/cruiser01_specular.png");

	firstMouse = true;
	
	cubemapTexture = Skybox::loadCubemap(faces);
	initAsteroids();

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
	glViewport(0, 0, width, height);
}

int main(int argc, char ** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(200, 200);
	glutInitWindowSize(600, 600);
	glutCreateWindow("OpenGL Pierwszy Program");
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