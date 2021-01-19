#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <vector>

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

const float RADIUS = 100.f;
const int ASTEROIDS_NUMBER = 60;

std::vector<GLuint> asteroidTextures;
std::vector<obj::Model> asteroidModels;
std::vector<Asteroid> asteroids;

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

//GLuint textureAsteroid;

bool firstMouse;
float newMouseX = 0;
float newMouseY = 0;
float lastX = 0;
float lastY = 0;
float mouseSpeed = 0.5f;

glm::quat rotationZ = glm::angleAxis(glm::radians(0.f), glm::vec3(0, 0, 1));

float skyboxVertices[] = {
	// positions          
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f
};
std::vector<std::string> faces
{
	"textures/space.jpg",
	"textures/space.jpg",
	"textures/space.jpg",
	"textures/space.jpg",
	"textures/space.jpg",
	"textures/space.jpg",
	//"textures/back_cp.jpeg",
	//"textures/back_cp.jpeg",
	//"textures/back_cp.jpeg",
	//"textures/back_cp.jpeg",
	//"textures/back_cp.jpeg",
	//"textures/back_cp.jpeg",

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
	/*case 'z': rotationZ *= glm::angleAxis(glm::radians(-angleSpeed), glm::vec3(0, 0, 1)); break;
	case 'x': rotationZ *= glm::angleAxis(glm::radians(angleSpeed), glm::vec3(0, 0, 1)); break;*/
	case 'z': rotation = glm::angleAxis(angleSpeed, glm::vec3(0, 0, 1))* rotation; break;
	case 'x': rotation = glm::angleAxis(angleSpeed, glm::vec3(0, 0, -1))* rotation; break;
	case 'w': cameraPos += cameraDir * moveSpeed; break;
	case 's': cameraPos -= cameraDir * moveSpeed; break;
	case 'd': cameraPos += cameraSide * moveSpeed; break;
	case 'a': cameraPos -= cameraSide * moveSpeed; break;
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

unsigned int loadCubemap(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void createCubemap() {
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	unsigned char* data;

	std::vector<std::string> textures_faces;
	textures_faces.push_back("GL_TEXTURE_CUBE_MAP_POSITIVE_X");
	textures_faces.push_back("GL_TEXTURE_CUBE_MAP_NEGATIVE_X");
	textures_faces.push_back("GL_TEXTURE_CUBE_MAP_POSITIVE_Y");
	textures_faces.push_back("GL_TEXTURE_CUBE_MAP_NEGATIVE_Y");
	textures_faces.push_back("GL_TEXTURE_CUBE_MAP_POSITIVE_Z");
	textures_faces.push_back("GL_TEXTURE_CUBE_MAP_NEAGTIVE_Z");

	for (unsigned int i = 0; i < textures_faces.size(); i++)
	{
		data = stbi_load(textures_faces[i].c_str(), &width, &height, &nrChannels, 0);
		glTexImage2D(
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
		);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

glm::mat4 createCameraMatrix()
{
	/*cameraDir = glm::vec3(cosf(cameraAngle - glm::radians(90.0f)), 0.0f, sinf(cameraAngle - glm::radians(90.0f)));
	glm::vec3 up = glm::vec3(0, 1, 0);
	cameraSide = glm::cross(cameraDir, up);*/

	/*return Core::createViewMatrix(cameraPos, cameraDir, up);*/
	

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
void renderScene()
{
	
	// Aktualizacja macierzy widoku i rzutowania
	cameraMatrix = createCameraMatrix();
	perspectiveMatrix = Core::createPerspectiveMatrix();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.1f, 0.3f, 1.0f);


	glm::mat4 shipInitialTransformation = glm::translate(glm::vec3(0, -0.25f, 0)) * glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.25f));
	glm::mat4 shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::mat4_cast(glm::inverse(rotation)) * shipInitialTransformation;
	drawObjectColor(&shipModel, shipModelMatrix, glm::vec3(0.6f));

	for (Asteroid asteroid : asteroids) {

		drawObjectTexture(&asteroid.Model, asteroid.Coordinates, asteroid.Texture);
	}

	
	/*glDepthFunc(GL_LEQUAL);
	glUseProgram(programSkybox);
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glm::mat4 view = glm::mat4(glm::mat3(cameraMatrix));
	glUniformMatrix4fv(glGetUniformLocation(programSkybox, "projection"), 1, GL_FALSE, (float*)&perspectiveMatrix);
	glUniformMatrix4fv(glGetUniformLocation(programSkybox, "view"), 1, GL_FALSE, (float*)&view);
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
	glBindBuffer(GL_ARRAY_BUFFER, 0);*/
	//Rendering Skybox
	
	//glDepthMask(GL_FALSE);
	glDepthFunc(GL_LEQUAL);
	glUseProgram(programSkybox);	
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	//glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	//glm::mat4 projection = glm::perspective(glm::radians(100.f), (float)600 / (float)600, 0.1f, 100.0f);
	//glm::mat4 view = glm::mat4(glm::mat3(cameraMatrix));
	glm::mat4 view = glm::mat4(glm::mat3(cameraMatrix));
	glUniformMatrix4fv(glGetUniformLocation(programSkybox, "projection"), 1, GL_FALSE, (float*)&perspectiveMatrix);
	glUniformMatrix4fv(glGetUniformLocation(programSkybox, "view"), 1, GL_FALSE, (float*)&view);	
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glDeleteBuffers(1, &skyboxVBO);
	glDeleteVertexArrays(1, &skyboxVAO);
	//glDeleteBuffers();
	glDepthFunc(GL_LESS);

	//glBindBuffer(GL_ARRAY_BUFFER, 0);


	glUseProgram(0);
	//glDepthMask(GL_TRUE);

	//glDepthRange(0.0, 0.9);
	//glClear(GL_DEPTH_BUFFER_BIT);
	
	glUseProgram(programStatic);
	glm::mat4 translation = glm::translate(glm::vec3(0.95f - 0.1f, -0.95f, 0.f));
	glUniformMatrix4fv(glGetUniformLocation(programStatic, "transformation"),
		1, GL_FALSE, (float*)&translation);
	drawHealth4(0.3f);
	glUseProgram(0);
	glutSwapBuffers();
}

void initAsteroids() {

	// ³adowanie textur asteroid
	for (const auto& file : fs::directory_iterator("textures/asteroids/"))
		asteroidTextures.push_back(Core::LoadTexture(file.path().string().c_str()));

	// ³adowanie dostêpnych modeli asteroid
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
	//programCubemap = shaderLoader.CreateProgram("shaders/shader_cubemap.vert", "shaders/shader_cubemap.frag");
	programSkybox = shaderLoader.CreateProgram("shaders/shader_skybox.vert", "shaders/shader_skybox.frag");
	programStatic = shaderLoader.CreateProgram("shaders/shader_hp.vert", "shaders/shader_hp.frag");
	sphereModel = obj::loadModelFromFile("models/sphere.obj");
	shipModel = obj::loadModelFromFile("models/spaceship.obj");
	firstMouse = true;
	
	cubemapTexture = loadCubemap(faces);
	initAsteroids();

	//Core::setA(1);
	//std::cout << Core::getA();
}

void shutdown()
{
	shaderLoader.DeleteProgram(programColor);
	shaderLoader.DeleteProgram(programTexture);
	shaderLoader.DeleteProgram(programSkybox);
	shaderLoader.DeleteProgram(programCubemap);
}

void idle()
{
	glutPostRedisplay();
}

void onReshape(int width, int height)
{
	//frustumScale = (float)width / (float)height;
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