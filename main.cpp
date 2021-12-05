#include <windows.h>
#define STB_IMAGE_IMPLEMENTATION
#include <glad/glad.h>
#include "SDL.h"
#include "SDL_opengl.h"
#include "FreeImage.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Include/Model.h"
#include "Include/camera.h"
#include "Include/IA.h"
#include "Include/ErrorsMSG.h"
#include <irrKlang.h>

#include <iostream>
#include <fstream>
#include "Config/tinyxml2.h"

#include <SDL_ttf.h>
#include <time.h>
#include <random>
#include <set>
using std::cerr;

#define SCR_H 720
#define SCR_W 1024

using namespace std;
using namespace irrklang;

tinyxml2::XMLNode* pEscena;
tinyxml2::XMLNode* pLucesHall;
tinyxml2::XMLNode* pLucesMapa;
tinyxml2::XMLNode* pLucesCharacter;
tinyxml2::XMLNode* pConfig;
tinyxml2::XMLElement* pLuz;
tinyxml2::XMLNode* pSonidos;
tinyxml2::XMLElement* pSonido;

// global variables - normally would avoid globals, using in this demo
GLuint shaderprogram; // handle for shader program
GLuint vao, vbo[2];	  // handles for our VAO and two VBOs
float r = 0;

unsigned int quadVAO = 0;
unsigned int quadVBO;

void renderQuad(float quadVertices[])
{
	{

		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, 80, &quadVertices[0], GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	}
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void cleanup(void)
{
	glUseProgram(0);
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);
	// could also detach shaders
	glDeleteProgram(shaderprogram);
	glDeleteBuffers(2, vbo);
	glDeleteVertexArrays(1, &vao);
}

struct movement
{
	bool spedUp = false;
	bool moving_forward = false;
	bool moving_back = false;
	bool moving_right = false;
	bool moving_left = false;
	bool moving_up = false;
	bool moving_down = false;
};

void move(movement mv, Camera* camera, float cameraSpeed, Model& ourModel, ISoundEngine*& engine, ISoundSource* pasos[], int& ultimoPaso, bool modoLibre)
{
	glm::vec3 cameraPos = camera->getPos();
	glm::vec3 camAux = cameraPos;
	if (mv.spedUp)
	{
		cameraSpeed = cameraSpeed * 2;
	}
	glm::vec2 direction;
	if (mv.moving_forward || mv.moving_back)
	{
		direction = glm::normalize(glm::vec2(camera->getFront().x, camera->getFront().z));
	}
	if (mv.moving_forward)
	{
		cameraPos.x += cameraSpeed * direction.x;
		cameraPos.z += cameraSpeed * direction.y;
		if (!engine->isCurrentlyPlaying(pasos[ultimoPaso]) && !modoLibre)
		{
			engine->play2D(pasos[(ultimoPaso + 1) % 8]);
			ultimoPaso = (ultimoPaso + 1) % 8;
		}
	}
	if (mv.moving_back)
	{
		cameraPos.x -= cameraSpeed * direction.x;
		cameraPos.z -= cameraSpeed * direction.y;
		if (!engine->isCurrentlyPlaying(pasos[ultimoPaso]) && !modoLibre)
		{
			engine->play2D(pasos[(ultimoPaso + 1) % 8]);
			ultimoPaso = (ultimoPaso + 1) % 8;
		}
	}
	if (mv.moving_left)
	{
		cameraPos -= glm::normalize(glm::cross(camera->getFront(), glm::vec3(0, 1, 0))) * cameraSpeed;
		if (!engine->isCurrentlyPlaying(pasos[ultimoPaso]) && !modoLibre)
		{
			engine->play2D(pasos[(ultimoPaso + 1) % 8]);
			ultimoPaso = (ultimoPaso + 1) % 8;
		}
	}
	if (mv.moving_right)
	{
		cameraPos += glm::normalize(glm::cross(camera->getFront(), glm::vec3(0, 1, 0))) * cameraSpeed;
		if (!engine->isCurrentlyPlaying(pasos[ultimoPaso]) && !modoLibre)
		{
			engine->play2D(pasos[(ultimoPaso + 1) % 8]);
			ultimoPaso = (ultimoPaso + 1) % 8;
		}
	}
	if (mv.moving_up || mv.moving_down)
	{
		engine->stopAllSoundsOfSoundSource(pasos[ultimoPaso]);
		if (mv.moving_up)
		{
			cameraPos.y += cameraSpeed;
		}
		if (mv.moving_down)
		{
			cameraPos.y -= cameraSpeed;
		}
	}
	if (ourModel.MeshCollision2(cameraPos))
	{
		cameraPos = camAux;
	}
	camera->setPos(cameraPos);
}

void setupLightsHall(Shader& ourShader)
{
	pLuz = pLucesHall->FirstChildElement("Luz");

	glm::vec3* posicion = new glm::vec3();
	glm::vec3* direction = new glm::vec3();
	glm::vec3 ambient = glm::vec3(0.0f);
	glm::vec3 diffuse = glm::vec3(1.0f);
	glm::vec3 specular = glm::vec3(1.0f);
	float constant = 1.0f;
	float linear = 0.0f;
	float quadratic = 1.0f;
	float cutOff = 8.0f;
	float outerCutOff = 50.0f;
	int i = 0;

	ourShader.setFloat("constantSpot", constant);
	ourShader.setFloat("linearSpot", linear);
	ourShader.setFloat("quadraticSpot", quadratic);
	ourShader.setFloat("cutOffSpot", glm::cos(glm::radians(cutOff)));
	ourShader.setFloat("outerCutOffSpot", glm::cos(glm::radians(outerCutOff)));

	while (pLuz != nullptr)
	{
		pLuz->QueryFloatAttribute("xPos", &posicion->x);
		pLuz->QueryFloatAttribute("yPos", &posicion->y);
		pLuz->QueryFloatAttribute("zPos", &posicion->z);
		pLuz->QueryFloatAttribute("xDir", &direction->x);
		pLuz->QueryFloatAttribute("yDir", &direction->y);
		pLuz->QueryFloatAttribute("zDir", &direction->z);

		string ligthName = "spotLights[" + std::to_string(i);

		ourShader.setVec3(ligthName + "].position", *posicion);
		ourShader.setVec3(ligthName + "].direction", *direction);

		pLuz = pLuz->NextSiblingElement("Luz");
		i++;
	}
}

void configLightsHall(Shader& ourShader, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular)
{
	ourShader.setVec3("diffuseSpot", diffuse);
	ourShader.setVec3("specularSpot", specular);
	ourShader.setVec3("ambientSpot", ambient);
}

void setupLightsCharacter(Shader& ourShader)
{
	pLuz = pLucesCharacter->FirstChildElement("Luz");

	glm::vec3* ambient = new glm::vec3();
	glm::vec3* diffuse = new glm::vec3();
	glm::vec3* specular = new glm::vec3();
	float* constant = new float();
	float* linear = new float();
	float* quadratic = new float();
	float* cutOff = new float();
	float* outerCutOff = new float();

	pLuz->QueryFloatAttribute("xAmb", &ambient->x);
	pLuz->QueryFloatAttribute("yAmb", &ambient->y);
	pLuz->QueryFloatAttribute("zAmb", &ambient->z);
	pLuz->QueryFloatAttribute("xDif", &diffuse->x);
	pLuz->QueryFloatAttribute("yDif", &diffuse->y);
	pLuz->QueryFloatAttribute("zDif", &diffuse->z);
	pLuz->QueryFloatAttribute("xSpec", &specular->x);
	pLuz->QueryFloatAttribute("ySpec", &specular->y);
	pLuz->QueryFloatAttribute("zSpec", &specular->z);
	pLuz->QueryFloatAttribute("constant", constant);
	pLuz->QueryFloatAttribute("linear", linear);
	pLuz->QueryFloatAttribute("quadratic", quadratic);
	pLuz->QueryFloatAttribute(".cutOff", cutOff);
	pLuz->QueryFloatAttribute("outerCutOff", outerCutOff);

	string ligthName = "characterLight";

	ourShader.setVec3(ligthName + ".ambient", *ambient);
	ourShader.setVec3(ligthName + ".diffuse", *diffuse);
	ourShader.setVec3(ligthName + ".specular", *specular);
	ourShader.setFloat(ligthName + ".constant", *constant);
	ourShader.setFloat(ligthName + ".linear", *linear);
	ourShader.setFloat(ligthName + ".quadratic", *quadratic);
	ourShader.setFloat(ligthName + ".cutOff", glm::cos(glm::radians(*cutOff)));
	ourShader.setFloat(ligthName + ".outerCutOff", glm::cos(glm::radians(*outerCutOff)));
}

void setupLightsMap(Shader& ourShader)
{
	pLuz = pLucesMapa->FirstChildElement("Luz");

	glm::vec3* posicion = new glm::vec3();
	glm::vec3* ambient = new glm::vec3();
	glm::vec3* diffuse = new glm::vec3();
	glm::vec3* specular = new glm::vec3();
	float* constant = new float();
	float* linear = new float();
	float* quadratic = new float();
	int i = 0;

	while (pLuz != nullptr)
	{
		pLuz->QueryFloatAttribute("xPos", &posicion->x);
		pLuz->QueryFloatAttribute("yPos", &posicion->y);
		pLuz->QueryFloatAttribute("zPos", &posicion->z);
		pLuz->QueryFloatAttribute("xAmb", &ambient->x);
		pLuz->QueryFloatAttribute("yAmb", &ambient->y);
		pLuz->QueryFloatAttribute("zAmb", &ambient->z);
		pLuz->QueryFloatAttribute("xDif", &diffuse->x);
		pLuz->QueryFloatAttribute("yDif", &diffuse->y);
		pLuz->QueryFloatAttribute("zDif", &diffuse->z);
		pLuz->QueryFloatAttribute("xSpec", &specular->x);
		pLuz->QueryFloatAttribute("ySpec", &specular->y);
		pLuz->QueryFloatAttribute("zSpec", &specular->z);
		pLuz->QueryFloatAttribute("constant", constant);
		pLuz->QueryFloatAttribute("linear", linear);
		pLuz->QueryFloatAttribute("quadratic", quadratic);

		string ligthName = "pointLights[" + std::to_string(i);

		ourShader.setVec3(ligthName + "].position", *posicion);
		ourShader.setVec3(ligthName + "].ambient", *ambient);
		ourShader.setVec3(ligthName + "].diffuse", *diffuse);
		ourShader.setVec3(ligthName + "].specular", *specular);
		ourShader.setFloat(ligthName + "].constant", *constant);
		ourShader.setFloat(ligthName + "].linear", *linear);
		ourShader.setFloat(ligthName + "].quadratic", *quadratic);

		pLuz = pLuz->NextSiblingElement("Luz");
		i++;
		posicion = new glm::vec3();
		ambient = new glm::vec3();
		diffuse = new glm::vec3();
		specular = new glm::vec3();
		constant = new float();
		linear = new float();
		quadratic = new float();
	}
}

void configLightsMap(Shader& ourShader, glm::vec3 diffuse, glm::vec3 specular)
{
	pLuz = pLucesMapa->FirstChildElement("Luz");

	int i = 0;
	while (pLuz != nullptr)
	{
		string ligthName = "pointLights[" + std::to_string(i);

		ourShader.setVec3(ligthName + "].diffuse", diffuse);
		ourShader.setVec3(ligthName + "].specular", specular);
		if (i == 1 || i == 3 || i == 4)
		{
			ourShader.setVec3(ligthName + "].diffuse", diffuse / 2.0f);
			ourShader.setVec3(ligthName + "].specular", specular / 2.0f);
		}
		if (i == 15)
		{
			ourShader.setVec3(ligthName + "].diffuse", diffuse / 3.0f);
			ourShader.setVec3(ligthName + "].specular", specular / 3.0f);
		}

		pLuz = pLuz->NextSiblingElement("Luz");
		i++;
	}
}

void iniciarSonidos(ISoundEngine*& engine)
{
	engine->setSoundVolume(0);

	pSonido = pSonidos->FirstChildElement("Sonido");

	glm::vec3* posicion = new glm::vec3();
	float* minVol = new float();
	const char* rut;

	while (pSonido != nullptr)
	{
		pSonido->QueryFloatAttribute("xPos", &posicion->x);
		pSonido->QueryFloatAttribute("yPos", &posicion->y);
		pSonido->QueryFloatAttribute("zPos", &posicion->z);
		rut = pSonido->Attribute("rut");
		pSonido->QueryFloatAttribute("minDist", minVol);

		ISound* aux = engine->play3D(rut, vec3df(posicion->x, posicion->y, posicion->z), true, false, true);
		aux->setMinDistance(*minVol);

		pSonido = pSonido->NextSiblingElement("Sonido");
		posicion = new glm::vec3();
		minVol = new float();
	}

	engine->setSoundVolume(0.8);
}

void pausarSonidos(ISoundEngine*& engine)
{

	pSonido = pSonidos->FirstChildElement("Sonido");
	const char* rut;

	while (pSonido != nullptr)
	{
		rut = pSonido->Attribute("rut");
		engine->stopAllSoundsOfSoundSource(engine->getSoundSource(rut));
		pSonido = pSonido->NextSiblingElement("Sonido");
	}
}

void cargarSonidoPasos(ISoundEngine*& engine, ISoundSource* pasos[8])
{

	ISoundSource* paso1 = engine->addSoundSourceFromFile("../Include/AudioClip/FootstepMetal01.wav");
	paso1->forceReloadAtNextUse();
	paso1->setDefaultVolume(0.5f);
	ISoundSource* paso2 = engine->addSoundSourceFromFile("../Include/AudioClip/FootstepMetal02.wav");
	paso2->forceReloadAtNextUse();
	paso2->setDefaultVolume(0.5f);
	ISoundSource* paso3 = engine->addSoundSourceFromFile("../Include/AudioClip/FootstepMetal03.wav");
	paso3->forceReloadAtNextUse();
	paso3->setDefaultVolume(0.5f);
	ISoundSource* paso4 = engine->addSoundSourceFromFile("../Include/AudioClip/FootstepMetal04.wav");
	paso4->forceReloadAtNextUse();
	paso4->setDefaultVolume(0.5f);
	ISoundSource* paso5 = engine->addSoundSourceFromFile("../Include/AudioClip/FootstepMetal05.wav");
	paso5->forceReloadAtNextUse();
	paso5->setDefaultVolume(0.5f);
	ISoundSource* paso6 = engine->addSoundSourceFromFile("../Include/AudioClip/FootstepMetal06.wav");
	paso6->forceReloadAtNextUse();
	paso6->setDefaultVolume(0.5f);
	ISoundSource* paso7 = engine->addSoundSourceFromFile("../Include/AudioClip/FootstepMetal07.wav");
	paso7->forceReloadAtNextUse();
	paso7->setDefaultVolume(0.5f);
	ISoundSource* paso8 = engine->addSoundSourceFromFile("../Include/AudioClip/FootstepMetal08.wav");
	paso8->forceReloadAtNextUse();
	paso8->setDefaultVolume(0.5f);

	pasos[0] = paso1;
	pasos[1] = paso2;
	pasos[2] = paso3;
	pasos[3] = paso4;
	pasos[4] = paso5;
	pasos[5] = paso6;
	pasos[6] = paso7;
	pasos[7] = paso8;
}

unsigned int loadCubemap(vector<std::string> faces)
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
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
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

// Main Menu Functions
float getOrientation(glm::vec2 A1, glm::vec2 A2, glm::vec2 A3)
{
	return (A1.x - A3.x) * (A2.y - A3.y) - (A1.y - A3.y) * (A2.x - A3.x);
}

bool isInside(float x, float y)
{
	glm::vec2 P = glm::vec2(x, y);

	glm::vec2 A1 = glm::vec2(445, 567);
	glm::vec2 A2 = glm::vec2(616, 499);
	glm::vec2 A3 = glm::vec2(444, 431);

	bool inside = false;
	float orientationT = getOrientation(A1, A2, A3);
	float orientationT1 = getOrientation(A1, A2, P);
	float orientationT2 = getOrientation(A2, A3, P);
	float orientationT3 = getOrientation(A3, A1, P);
	if (orientationT >= 0)
	{
		inside = (orientationT1 >= 0) && (orientationT2 >= 0) && (orientationT3 >= 0);
	}
	else
	{
		inside = (orientationT1 < 0) && (orientationT2 < 0) && (orientationT3 < 0);
	}

	return inside;
}

int getOptionClicked(float x, float y) {
	glm::vec2 op1P1 = glm::vec2(312, 230);
	glm::vec2 op1P2 = glm::vec2(502, 206);

	glm::vec2 op2P1 = glm::vec2(312, 304);
	glm::vec2 op2P2 = glm::vec2(417, 277);

	glm::vec2 op3P1 = glm::vec2(312, 376);
	glm::vec2 op3P2 = glm::vec2(554, 353);

	glm::vec2 op4P1 = glm::vec2(312, 446);
	glm::vec2 op4P2 = glm::vec2(553, 418);

	glm::vec2 op5P1 = glm::vec2(312, 518);
	glm::vec2 op5P2 = glm::vec2(468, 491);

	glm::vec2 op6P1 = glm::vec2(309, 590);
	glm::vec2 op6P2 = glm::vec2(431, 567);

	if (op1P1.x <= x && x <= op1P2.x && op1P1.y >= y && y >= op1P2.y) {
		return 1;
	}
	if (op2P1.x <= x && x <= op2P2.x && op2P1.y >= y && y >= op2P2.y) {
		return 2;
	}
	if (op3P1.x <= x && x <= op3P2.x && op3P1.y >= y && y >= op3P2.y) {
		return 3;
	}
	if (op4P1.x <= x && x <= op4P2.x && op4P1.y >= y && y >= op4P2.y) {
		return 4;
	}
	if (op5P1.x <= x && x <= op5P2.x && op5P1.y >= y && y >= op5P2.y) {
		return 5;
	}
	if (op6P1.x <= x && x <= op6P2.x && op6P1.y >= y && y >= op6P2.y) {
		return 6;
	}
	return 0;
}


// Two Factor Task Functions
char getDigit(float x, float y)
{
	float ratio = 32.0;
	glm::vec2 p = glm::vec2(x, y);
	glm::vec2 num1 = glm::vec2(294, 325);
	glm::vec2 num2 = glm::vec2(382, 325);
	glm::vec2 num3 = glm::vec2(466, 325);

	glm::vec2 num4 = glm::vec2(294, 402);
	glm::vec2 num5 = glm::vec2(382, 402);
	glm::vec2 num6 = glm::vec2(466, 402);

	glm::vec2 num7 = glm::vec2(294, 482);
	glm::vec2 num8 = glm::vec2(382, 482);
	glm::vec2 num9 = glm::vec2(466, 482);

	glm::vec2 num0 = glm::vec2(382, 564);

	char digit = ' ';
	if (distance(p, num1) <= ratio)
	{
		digit = '1';
	}
	else if (distance(p, num2) <= ratio)
	{
		digit = '2';
	}
	else if (distance(p, num3) <= ratio)
	{
		digit = '3';
	}
	else if (distance(p, num4) <= ratio)
	{
		digit = '4';
	}
	else if (distance(p, num5) <= ratio)
	{
		digit = '5';
	}
	else if (distance(p, num6) <= ratio)
	{
		digit = '6';
	}
	else if (distance(p, num7) <= ratio)
	{
		digit = '7';
	}
	else if (distance(p, num8) <= ratio)
	{
		digit = '8';
	}
	else if (distance(p, num9) <= ratio)
	{
		digit = '9';
	}
	else if (distance(p, num0) <= ratio)
	{
		digit = '0';
	}
	return digit;
}

bool getConfirmPass(float x, float y)
{
	float ratio = 32.0;
	glm::vec2 p = glm::vec2(x, y);
	glm::vec2 confirm = glm::vec2(466, 564);
	return (distance(p, confirm) <= ratio);
}

bool getErasePass(float x, float y)
{
	float ratio = 32.0;
	glm::vec2 p = glm::vec2(x, y);
	glm::vec2 confirm = glm::vec2(294, 564);
	return (distance(p, confirm) <= ratio);
}

unsigned int loadTexture(string path)
{
	unsigned int texture1;
	// texture 1
	// ---------
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, STBI_rgb_alpha);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);
	return texture1;
}

void renderQuad(Shader shader, glm::vec2 top_left, glm::vec2 bottom_right, unsigned int useTex)
{
	float coords[] = {
		// positions        // texture Coords
		top_left.x,
		top_left.y,
		0.0f,
		0.0f,
		1.0f,
		top_left.x,
		bottom_right.y,
		0.0f,
		0.0f,
		0.0f,
		bottom_right.x,
		top_left.y,
		0.0f,
		1.0f,
		1.0f,
		bottom_right.x,
		bottom_right.y,
		0.0f,
		1.0f,
		0.0f,
	};
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content
	glDisable(GL_DEPTH_TEST);
	shader.use();
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, useTex);
	renderQuad(coords);
}

void renderQuad(Shader shader, float coords[], unsigned int useTex)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content
	glDisable(GL_DEPTH_TEST);
	shader.use();
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, useTex);
	renderQuad(coords);
}
bool inRange(glm::vec3 pos, vector<glm::vec3> pointCollection, float distance)
{
	for (glm::vec3 comp : pointCollection)
	{
		if (glm::distance(comp, pos) < distance)
			return true;
	}
	return false;
}

glm::vec2 getScaledCoords(float x, float y)
{
	return glm::vec2((2.f * x / SCR_W) - 1.f, ((-2.f * y / SCR_H) + 1.f));
}
glm::vec2 getScaledCoords(glm::vec2 vec)
{
	return glm::vec2((2.f * vec.x / SCR_W) - 1.f, ((-2.f * vec.y / SCR_H) + 1.f));
}

vector<unsigned int> wires;
unsigned int cablesTex;
vector<int> colors_left{ 0, 1, 2, 3 };
vector<int> colors_right{ 0, 1, 2, 3 };
vector<bool> fixed_cables(4, false);
bool panel_inPos = false;
int currentPos = 0;
int displacementScaling = 120;

vector<glm::vec2> leftCablePos{
	glm::vec2(210, 227),
	glm::vec2(210, 317),
	glm::vec2(210, 406),
	glm::vec2(210, 495) };

vector<glm::vec2> rightCablePos{
	glm::vec2(774, 227),
	glm::vec2(774, 317),
	glm::vec2(774, 406),
	glm::vec2(774, 495) };

glm::vec2 cableSize(38, 17);


int closestIndex(glm::vec3 pos, vector<glm::vec3> pointCollection, float distance)
{
	for (int i = 0; i < pointCollection.size(); i++)
	{
		if (glm::distance((pointCollection[i]), pos) < distance)
			return i;
	}
	return -1;
}

int closestIndex(glm::vec2 pos, vector<glm::vec2> pointCollection, float distance)
{
	for (int i = 0; i < pointCollection.size(); i++)
	{
		if (glm::distance(getScaledCoords(pointCollection[i]), pos) < distance)
			return i;
	}
	return -1;
}

template <typename T>
int getIndex(vector<T> v, T elem)
{
	auto it = find(v.begin(), v.end(), elem);
	// If element was found
	if (it != v.end())
	{
		int index = it - v.begin();
		return index;
	}
	else
	{
		return -1;
	}
}

void playCables(Shader shader, glm::vec2 last_click, glm::vec2 mouse_pos, bool clicked, bool reset, ISoundEngine* engine, ISoundSource* wireSound, int diff)
{
	if (reset)
	{
		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(colors_left.begin(), colors_left.end(), g);
		std::shuffle(colors_right.begin(), colors_right.end(), g);
		fixed_cables = vector<bool>(4, false);
		panel_inPos = false;
		currentPos = 0;
	}

	glm::vec2 top_left;
	glm::vec2 bottom_right;
	if (!panel_inPos) {
		int displacement = SCR_H * 4 / 5 - (((SCR_H * 4 / 5)) / displacementScaling * currentPos);
		top_left = getScaledCoords(SCR_W / 5, (SCR_H / 5) + displacement);
		bottom_right = getScaledCoords(SCR_W * 4 / 5, (SCR_H * 4 / 5) + displacement);
		currentPos += 15 * diff;
		panel_inPos = currentPos >= displacementScaling;
	}
	else {
		top_left = getScaledCoords(SCR_W / 5, SCR_H / 5);
		bottom_right = getScaledCoords(SCR_W * 4 / 5, SCR_H * 4 / 5);
	}

	renderQuad(shader, top_left, bottom_right, cablesTex);


	if (panel_inPos) {
		for (int i = 0; i < 4; i++)
		{
			renderQuad(shader, getScaledCoords(leftCablePos[i]), getScaledCoords(leftCablePos[i] + cableSize), wires[colors_left[i]]);
			renderQuad(shader, getScaledCoords(rightCablePos[i]), getScaledCoords(rightCablePos[i] + cableSize), wires[colors_right[i]]);
		}

		if (clicked)
		{
			int cableId = closestIndex(last_click, leftCablePos, 0.09);
			if (cableId > -1 && !fixed_cables[cableId])
			{
				glm::vec2 origin = getScaledCoords(leftCablePos[cableId] + glm::vec2(cableSize.x, 0));
				float width = 0.05;
				float coords[] = {
					// positions        // texture Coords
					origin.x,origin.y,0.0f,0.0f,1.0f,
					origin.x,origin.y - width,0.0f,0.0f,0.0f,
					mouse_pos.x,mouse_pos.y,0.0f,1.0f,1.0f,
					mouse_pos.x,mouse_pos.y - width,0.0f,1.0f,0.0f,
				};
				renderQuad(shader, coords, wires[colors_left[cableId]]);
				int targetId = closestIndex(mouse_pos, rightCablePos, 0.05);
				if (colors_right[targetId] == colors_left[cableId])
				{
					fixed_cables[cableId] = true;
					engine->play2D(wireSound);
				}
			}
		}
		for (int cable = 0; cable < 4; cable++)
		{
			if (fixed_cables[cable])
			{
				glm::vec2 origin = getScaledCoords(leftCablePos[cable] + glm::vec2(cableSize.x, 0));
				glm::vec2 destination = getScaledCoords(rightCablePos[getIndex(colors_right, colors_left[cable])]);
				float width = 0.048;
				float coords[] = {
					// positions        // texture Coords
					origin.x,
					origin.y,
					0.0f,
					0.0f,
					1.0f,
					origin.x,
					origin.y - width,
					0.0f,
					0.0f,
					0.0f,
					destination.x,
					destination.y,
					0.0f,
					1.0f,
					1.0f,
					destination.x,
					destination.y - width,
					0.0f,
					1.0f,
					0.0f,
				};
				renderQuad(shader, coords, wires[colors_left[cable]]);
			}
		}
	}
}

void renderF(Shader shader, unsigned int tex)
{
	glm::vec2 top_left = getScaledCoords(820, 520);
	glm::vec2 bottom_right = getScaledCoords(1024, 720);
	renderQuad(shader, top_left, bottom_right, tex);
}

vector<glm::vec3> cableSpots{
	glm::vec3(26.6652f, 0.31f, -11.6247f),
	glm::vec3(44.9004f, 0.31f, -16.924f),
	glm::vec3(23.1515f, 0.31f, -27.1345f),
	glm::vec3(20.628f, 0.31f, -12.8736f),
	glm::vec3(11.8638f, 0.31f, -16.0048f) };

vector<glm::vec2> cableSpotsMap{
	glm::vec2(504, 388),
	glm::vec2(813, 299),
	glm::vec2(439, 108),
	glm::vec2(404, 366),
	glm::vec2(241, 316) };

vector<glm::vec3> oxygenSpots{
	glm::vec3(36.5912f, 0.31f, -17.9872f),
	glm::vec3(36.2865f, 0.31f, -14.1531f),
};

vector<glm::vec2> oxygenSpotsMap{
	glm::vec2(691, 283),
	glm::vec2(686, 357),
};

template <typename T>
std::vector<T> Append(std::vector<T>& a, const std::vector<T>& b)
{
	std::vector<T> c(a);
	c.insert(c.end(), b.begin(), b.end());
	return c;
}


vector<int> randomUnique(int n, int max) {
	std::set<int> numbers;
	while (numbers.size() < n)
	{
		numbers.insert(rand() % max);
	}
	return std::vector<int>(numbers.begin(), numbers.end());
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*																	|							 |
																	|			MAIN			 |
																	|							 |
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{

	// INITIALIZATION
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
	{
		SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
		return 1;
	}

	// Load xml file
	tinyxml2::XMLDocument xmlDoc;
	xmlDoc.LoadFile("../Config/Escena/escena.xml");
	pEscena = xmlDoc.FirstChild();
	pConfig = pEscena->FirstChildElement("Config");
	pLucesHall = pEscena->FirstChildElement("LucesHall");
	pLucesMapa = pEscena->FirstChildElement("LucesMapa");
	pLucesCharacter = pEscena->FirstChildElement("LucesCharacter");
	pSonidos = pEscena->FirstChildElement("Sonidos");

	SDL_GLContext gl_context;
	SDL_Window* window = NULL;
	float maxSamples;
	bool activate = false;
	tinyxml2::XMLElement* pMSAA = pConfig->FirstChildElement("MSAA");
	pMSAA->QueryFloatAttribute("samples", &maxSamples);
	if (maxSamples > 16)
		maxSamples = 16;
	if (maxSamples < 2)
		maxSamples = 2;

	float amount;
	tinyxml2::XMLElement* pBloom = pConfig->FirstChildElement("BLOOM");
	pBloom->QueryFloatAttribute("amount", &amount);

	// Config Multisample Render
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, maxSamples);

	window = SDL_CreateWindow("EntreNosotros3D", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		SCR_W, SCR_H, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if (TTF_Init() == 1)
	{
		cout << "TTF_Init() Failed: " << TTF_GetError() << endl;
		SDL_Quit();
		exit(1);
	}
	TTF_Font* font;
	font = TTF_OpenFont("../Include/FreeSans.ttf", 20);
	if (font == NULL)
	{
		cout << "TTF_OpenFont() Failed: " << TTF_GetError() << endl;
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}

	TTF_Font* digitalFont;
	digitalFont = TTF_OpenFont("../Include/DigitalFont.ttf", 60);
	if (font == NULL)
	{
		cout << "TTF_OpenFont() Failed: " << TTF_GetError() << endl;
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}

	TTF_Font* gameFont;
	gameFont = TTF_OpenFont("../Include/VarelaRound-Regular.otf", 60);
	if (gameFont == NULL)
	{
		cout << "TTF_OpenFont() Failed: " << TTF_GetError() << endl;
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}
	TTF_Font* gameFontOutline = TTF_OpenFont("../Include/VarelaRound-Regular.otf", 60);
	if (gameFontOutline == NULL)
	{
		cout << "TTF_OpenFont() Failed: " << TTF_GetError() << endl;
		TTF_Quit();
		SDL_Quit();
		exit(1);
	}
	TTF_SetFontOutline(gameFontOutline, 6);

	// Write text to surface
	SDL_Color text_color = { 64, 132, 150 };

	SDL_Color white_color = { 255, 255, 255 };

	SDL_Color black_color = { 0, 0, 0 };

	SDL_Color green_color = { 0, 255, 0 };

	SDL_CaptureMouse(SDL_TRUE);
	SDL_ShowCursor(SDL_DISABLE);
	gl_context = SDL_GL_CreateContext(window);

	// Disable limit of 60fps
	SDL_GL_SetSwapInterval(0);

	gladLoadGLLoader(SDL_GL_GetProcAddress);

	// Check OpenGL properties
	printf("OpenGL loaded\n");
	printf("Vendor:   %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version:  %s\n", glGetString(GL_VERSION));
	glEnable(GL_DEPTH_TEST); // enable depth testing
	glEnable(GL_CULL_FACE);	 // enable back face culling - try this and see what happens!
	glEnable(GL_DEPTH_CLAMP);
	glEnable(GL_BLEND);
	glDisable(GL_MULTISAMPLE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // enable depth testing

	// Setup Shaders
	Shader ourShader("../Shaders/model_loading.vs", "../Shaders/model_loading.fs");
	Shader skyboxShader("../Shaders/skybox_render.vs", "../Shaders/skybox_render.fs");
	Shader blur("../Shaders/blur.vs", "../Shaders/blur.fs");
	Shader bloomFinal("../Shaders/blur.vs", "../Shaders/bloom_final.fs");
	Shader shadowMapProgram("../Shaders/shadow.vs", "../Shaders/shadow.fs");
	Shader ShadowDebug("../Shaders/debugShadow.vs", "../Shaders/debugShadow.fs");

	blur.use();
	blur.setInt("image", 0);
	bloomFinal.use();
	bloomFinal.setInt("scene", 0);
	bloomFinal.setInt("bloomBlur", 1);

	ShadowDebug.use();
	ShadowDebug.setInt("game", 1);
	float alpha = 1.0;
	ShadowDebug.setFloat("alpha", alpha);
	ShadowDebug.setBool("transparencyIsAvailable", true);

	skyboxShader.use();
	skyboxShader.setInt("sun", 0);
	skyboxShader.setInt("skybox", 1);

	//stbi_set_flip_vertically_on_load(true);

	// SETUP MODELS
	Model ourModel("../Include/model/c.obj");
	Model cuerpo1("../Include/model/astronaut.dae");
	Model muerto("../Include/model/dead.obj");
	Model sun("../Include/model/sun.obj");
	Model fantasma("../Include/model/ghost.dae");

	// Setup model Octree
	ourModel.GenerateOctree();

	// Setup cubeMap
	vector<std::string> faces{
		"../Include/skybox/right.png",
		"../Include/skybox/left.png",
		"../Include/skybox/top.png",
		"../Include/skybox/bottom.png",
		"../Include/skybox/front.png",
		"../Include/skybox/back.png" };

	float skyboxVertices[] = {
		// positions
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,

		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f };

	float upRight[] = {
		// positions        // texture Coords
		0.0f,1.0f,0.0f,0.0f,1.0f,
		0.0f,0.0f,0.0f,0.0f,0.0f,
		1.0f,1.0f,0.0f,1.0f,1.0f,
		1.0f,0.0f,0.0f,1.0f,0.0f,
	};

	float full[] = {
		// positions        // texture Coords
		-1.0f,1.0f,0.0f,0.0f,1.0f,
		-1.0f,-1.0f,0.0f,0.0f,0.0f,
		1.0f,1.0f,0.0f,1.0f,1.0f,
		1.0f,-1.0f,0.0f,1.0f,0.0f,
	};

	float up_left[] = {
		// positions        // texture Coords
		-0.992f,1.0f,0.0f,0.0f,0.0f,
		-0.992f,0.9f,0.0f,0.0f,1.0f,
		-0.85f,1.0f,0.0f,1.0f,0.0f,
		-0.85f,0.9f,0.0f,1.0f,1.0f,
	};

	// Task Complete Position

	float taskCompletePosition[] = {
		// positions        // texture Coords
		0.46289,0.082,0.0f,1.0f,0.0f,
		-0.40429,0.082,0.0f,0.0f,0.0f,
		0.46289,-0.12,0.0f,1.0f,1.0f,
		-0.40429,-0.12,0.0f,0.0f,1.0f,
	};

	// Two Factor Task Positions

	float twoFactorPosition[] = {
		// positions        // texture Coords
		0.03125,
		0.671111,
		0.0f,
		1.0f,
		0.0f,
		-0.541016,
		0.671111,
		0.0f,
		0.0f,
		0.0f,
		0.03125,
		0.540556,
		0.0f,
		1.0f,
		1.0f,
		-0.541016,
		0.540556,
		0.0f,
		0.0f,
		1.0f,
	};

	float twoFactorPositionPass[] = {
		// positions        // texture Coords
		0.291016,
		0.133333,
		0.0f,
		0.0f,
		1.0f,
		0.488281,
		0.133333,
		0.0f,
		1.0f,
		1.0f,
		0.291016,
		0.238889,
		0.0f,
		0.0f,
		0.0f,
		0.488281,
		0.238889,
		0.0f,
		1.0f,
		0.0f,
	};

	float twoFactorStatusPosition[] = {
		// positions        // texture Coords
		0.03125,
		0.383111,
		0.0f,
		1.0f,
		0.0f,
		-0.541016,
		0.383111,
		0.0f,
		0.0f,
		0.0f,
		0.03125,
		0.252556,
		0.0f,
		1.0f,
		1.0f,
		-0.541016,
		0.252556,
		0.0f,
		0.0f,
		1.0f,
	};

	// Option Menu Positions

	float optionMenuPositions[] = {
		// positions        // texture Coords
		-0.8f + 0.3,0.8f,0.0f,0.0f,1.0f,
		-0.8f + 0.3,-0.8f,0.0f,0.0f,0.0f,
		0.2f + 0.3,0.8f,0.0f,1.0f,1.0f,
		0.2f + 0.3,-0.8f,0.0f,1.0f,0.0f,
	};

	float optionMenuTitlePositions[] = {
		// positions        // texture Coords
		0.06289f + 0.3,0.732f,0.0f,1.0f,0.0f,
		-0.60429 + 0.3,0.732f,0.0f,0.0f,0.0f,
		0.06289f + 0.3,0.58f,0.0f,1.0f,1.0f,
		-0.60429 + 0.3,0.58f,0.0f,0.0f,1.0f,
	};

	float optionOnePositions[] = {
		// positions        // texture Coords
		-0.3125 + 0.3,0.461111,0.0f,1.0f,0.0f,
		-0.703125 + 0.3,0.461111,0.0f,0.0f,0.0f,
		-0.3125 + 0.3,0.333333,0.0f,1.0f,1.0f,
		-0.703125 + 0.3,0.333333,0.0f,0.0f,1.0f,
	};

	float optionTwoPositions[] = {
		// positions        // texture Coords
		-0.3125 + 0.3,0.461111 - 0.2,0.0f,1.0f,0.0f,
		-0.703125 + 0.3,0.461111 - 0.2,0.0f,0.0f,0.0f,
		-0.3125 + 0.3,0.333333 - 0.2 + 0.01,0.0f,1.0f,1.0f,
		-0.703125 + 0.3,0.333333 - 0.2 + 0.01,0.0f,0.0f,1.0f,
	};

	float optionThreePositions[] = {
		// positions        // texture Coords
		-0.3125 + 0.4,0.461111 - 0.4,0.0f,1.0f,0.0f,
		-0.703125 + 0.3,0.461111 - 0.4,0.0f,0.0f,0.0f,
		-0.3125 + 0.4,0.333333 - 0.406,0.0f,1.0f,1.0f,
		-0.703125 + 0.3,0.333333 - 0.406,0.0f,0.0f,1.0f,
	};

	float optionFourPositions[] = {
		// positions        // texture Coords
		-0.3125 + 0.4,0.461111 - 0.6,0.0f,1.0f,0.0f,
		-0.703125 + 0.3,0.461111 - 0.6,0.0f,0.0f,0.0f,
		-0.3125 + 0.4,0.333333 - 0.6,0.0f,1.0f,1.0f,
		-0.703125 + 0.3,0.333333 - 0.6,0.0f,0.0f,1.0f,
	};

	float optionFivePositions[] = {
		// positions        // texture Coords
		-0.3125 + 0.3,0.461111 - 0.8,0.0f,1.0f,0.0f,
		-0.703125 + 0.3,0.461111 - 0.8,0.0f,0.0f,0.0f,
		-0.3125 + 0.3,0.333333 - 0.8 + 0.013,0.0f,1.0f,1.0f,
		-0.703125 + 0.3,0.333333 - 0.8 + 0.013,0.0f,0.0f,1.0f,
	};

	float optionSixPositions[] = {
		// positions        // texture Coords
		-0.3125 + 0.3,0.461111 - 1.0,0.0f,1.0f,0.0f,
		-0.703125 + 0.3,0.461111 - 1.0,0.0f,0.0f,0.0f,
		-0.3125 + 0.3,0.333333 - 1.0 + 0.013,0.0f,1.0f,1.0f,
		-0.703125 + 0.3,0.333333 - 1.0 + 0.013,0.0f,0.0f,1.0f,
	};

	// load textures
	unsigned int cubemapTexture = loadCubemap(faces);
	unsigned int gameTexture = loadTexture("../Include/model/mapa-png.png");
	unsigned int mapMarker = loadTexture("../Include/model/marker.png");
	unsigned int mainMenu = loadTexture("../Include/model/main-menu.png");
	unsigned int blackWindows = loadTexture("../Include/model/black-windows.png");
	unsigned int windowsMenuOptions = loadTexture("../Include/model/optionMenu.png");
	unsigned int deadTexture = loadTexture("../Include/model/killBG.png");
	unsigned int game_over = loadTexture("../Include/model/derrota.png");
	unsigned int game_victory = loadTexture("../Include/model/victoria.png");
	unsigned int icon_error = loadTexture("../Include/model/IconError.png");
	unsigned int twoFactorBase = loadTexture("../Include/minigames/2factor_base.png");

	cablesTex = loadTexture("../Include/minigames/wirepanel.png");
	unsigned int rwire = loadTexture("../Include/minigames/rwire.png");
	unsigned int bwire = loadTexture("../Include/minigames/bwire.png");
	unsigned int ywire = loadTexture("../Include/minigames/ywire.png");
	unsigned int pwire = loadTexture("../Include/minigames/pwire.png");

	wires.push_back(rwire);
	wires.push_back(bwire);
	wires.push_back(ywire);
	wires.push_back(pwire);

	unsigned int use = loadTexture("../Include/minigames/use.png");

	// INITIALIZE VARIABLES
	SDL_DisplayMode DM;
	SDL_Event sdlEvent; // variable to detect SDL events

	// Time tracking
	Uint32 deltaTime = 0; // Time between current frame and last frame
	Uint32 lastFrame = 0; // Time of last frame
	short fps_to_show = 10;

	// Cam Rotation
	float yaw = -90.0f;
	float pitch = 0.0f;
	float speed = 2.5f;
	float sensitivity = 0.1f;
	float zoom = 45.0f;
	Camera* camera = new Camera();
	camera->Rotate(yaw, pitch);

	// View, Model and Projection Matrix
	glm::mat4 modelCadaver = glm::mat4(1.f);
	modelCadaver = glm::translate(modelCadaver, glm::vec3(21.0f, 0.0f, -12.6f));
	modelCadaver = glm::scale(modelCadaver, glm::vec3(0.3f, 0.3f, 0.3f));
	modelCadaver = glm::mat4(glm::rotate(modelCadaver, glm::radians(-90.0f), glm::vec3(0.0, 1.0, 0.0)));
	glm::mat4 matr_normals = glm::mat4(glm::transpose(glm::inverse(modelCadaver)));

	glm::mat4 modelFantasma = glm::mat4(1.0f);
	glm::mat4 rotatematrix = glm::mat4(1.0f);
	modelFantasma = glm::translate(modelFantasma, glm::vec3(20.3f, 0.2f, -12.70f));
	modelFantasma = glm::scale(modelFantasma, glm::vec3(0.15f, 0.15f, 0.15f));
	IA* ghost = new IA();

	glm::mat4 view, model, projection, modelsun;
	glm::mat4 modelAnim = glm::mat4(1.0f);
	modelAnim = glm::translate(modelAnim, glm::vec3(29.26f, 0.0f, -24.32f));
	modelAnim = glm::scale(modelAnim, glm::vec3(0.2f, 0.2f, 0.2f));
	glm::mat4 medicRot = glm::mat4(1.0f);

	modelsun = glm::mat4(1.0f);
	modelsun = glm::translate(modelsun, glm::vec3(25.0f, 20.0f, 35.0f)); //modelsun = glm::translate(modelsun, glm::vec3(25.0f, 35.0f, 35.0f)); //glm::vec3(25.0f, 20.0f, 50.0f));
	modelsun = glm::scale(modelsun, glm::vec3(0.3f, 0.3f, 0.3f));

	// GAME STATES

	enum STATES
	{
		INIT,
		MAIN_MENU,
		TRANSITION,
		GAME,
		COMPLETED,
		END_GAME
	};

	STATES actualState = INIT;
	STATES previousState = TRANSITION;
	vector<bool> completedMissions(5, false);
	vector <int> assignedWire(3);

	// Task Variables
	// Two Factor
	int twoFactorMission;
	bool isTwoFactorTask = false;
	char twoFactorNumbers[5] = { ' ', ' ', ' ', ' ', ' ' };
	string twoFactorPass = "12345"; // default
	bool confirmPass = false;
	int timeVisibleState = 0;

	// Wire
	int currentMission;
	int missionIndex;
	bool cables = false;
	bool btn_down = false;
	bool reset = false;
	glm::vec2 last_click = glm::vec2(0.f);
	glm::vec2 mouse_pos = glm::vec2(0.f);

	// Scene variables
	SDL_ShowCursor(SDL_ENABLE);
	bool soundScan = true;
	bool soundKill = true;
	bool optionsMenu = false;
	bool sonido = false;
	bool soundConfirmTask = true;
	bool soundTaskComplete = true;
	bool running = true;
	bool fullScreen = false;
	bool bloom = true;
	bool antialiasing = true;
	bool dibujo = true;
	float exposure = 2.0f;
	bool cortoElectricidad = false;
	bool apagon = false;
	bool linterna = false;
	bool specular_map = false;
	bool fixed_pos = false;
	bool first_person = false;
	bool renderMap = true;
	bool renderMapComplete = false;
	bool shadowMapEnabled = false;
	float old_yaw = 0.f;
	glm::vec3 old_pos = glm::vec3(0.f);
	glm::vec3 old_pos_camera = camera->getPos();
	glm::vec3 old_front_camera = camera->getFront();
	movement mv;
	bool lock_cam = false;
	bool moved = false;
	int count = 0;
	int transitionCounter = 0;
	int timeAux = 0;
	int diff = 0;
	int timeN = 0;
	bool se_activa_el_fantasma = false;
	bool medic_Scan = false;
	bool medic_Scan_available = true;
	float wasted = -1.f;

	// Setup lights
	glm::vec3 ambient = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 diffuse = glm::vec3(0.8f);
	glm::vec3 specular = glm::vec3(0.6f);
	glm::vec3 diffuseHall = glm::vec3(1.0f, 1.0f, 1.0f);
	glm::vec3 specularHall = glm::vec3(1.0f, 1.0f, 1.0f);

	ourShader.use();
	setupLightsMap(ourShader);
	setupLightsCharacter(ourShader);
	setupLightsHall(ourShader);

	// Setup sounds
	ISoundEngine* engine = createIrrKlangDevice();

	ISoundSource* sirenSound = engine->addSoundSourceFromFile("../Include/AudioClip/SabotageSiren.wav");
	sirenSound->setDefaultVolume(0.2f);
	sirenSound->forceReloadAtNextUse();
	ISoundSource* ligthOffSound = engine->addSoundSourceFromFile("../Include/AudioClip/panel_reactor_manifoldfail.wav");
	ligthOffSound->setDefaultVolume(1.0f);
	ligthOffSound->forceReloadAtNextUse();
	ISoundSource* mainMenuSound = engine->addSoundSourceFromFile("../Include/AudioClip/AmongUsTheme.wav");
	mainMenuSound->setDefaultVolume(0.2f);
	mainMenuSound->forceReloadAtNextUse();
	ISoundSource* playerSpawnSound = engine->addSoundSourceFromFile("../Include/AudioClip/Player_Spawn.wav");
	playerSpawnSound->setDefaultVolume(0.2f);
	playerSpawnSound->forceReloadAtNextUse();
	ISoundSource* uiSelectSound = engine->addSoundSourceFromFile("../Include/AudioClip/UI_Select.wav");
	uiSelectSound->setDefaultVolume(0.2f);
	uiSelectSound->forceReloadAtNextUse();
	ISoundSource* enterIdSound = engine->addSoundSourceFromFile("../Include/AudioClip/panel_enterID.wav");
	enterIdSound->setDefaultVolume(0.2f);
	enterIdSound->forceReloadAtNextUse();
	ISoundSource* enterIdBadSound = engine->addSoundSourceFromFile("../Include/AudioClip/panel_enterIDBad.wav");
	enterIdBadSound->setDefaultVolume(0.2f);
	enterIdBadSound->forceReloadAtNextUse();
	ISoundSource* enterIdGoodSound = engine->addSoundSourceFromFile("../Include/AudioClip/panel_enterIDGood.wav");
	enterIdGoodSound->setDefaultVolume(0.2f);
	enterIdGoodSound->forceReloadAtNextUse();
	ISoundSource* panelAppearSound = engine->addSoundSourceFromFile("../Include/AudioClip/Panel_GenericAppear.wav");
	panelAppearSound->setDefaultVolume(0.2f);
	panelAppearSound->forceReloadAtNextUse();
	ISoundSource* panelDisappearSound = engine->addSoundSourceFromFile("../Include/AudioClip/Panel_GenericDisappear.wav");
	panelDisappearSound->setDefaultVolume(0.2f);
	panelDisappearSound->forceReloadAtNextUse();
	ISoundSource* wireSound = engine->addSoundSourceFromFile("../Include/AudioClip/panel_electrical_wire.wav");
	wireSound->setDefaultVolume(0.2f);
	wireSound->forceReloadAtNextUse();
	ISoundSource* taskCompleteSound = engine->addSoundSourceFromFile("../Include/AudioClip/task_Complete.wav");
	taskCompleteSound->setDefaultVolume(0.2f);
	taskCompleteSound->forceReloadAtNextUse();
	ISoundSource* optionClickedSound = engine->addSoundSourceFromFile("../Include/AudioClip/panel_drillButton.wav");
	optionClickedSound->setDefaultVolume(0.2f);
	optionClickedSound->forceReloadAtNextUse();
	ISoundSource* roundStartSound = engine->addSoundSourceFromFile("../Include/AudioClip/Roundstart_MAIN.wav");
	roundStartSound->setDefaultVolume(0.2f);
	roundStartSound->forceReloadAtNextUse();
	ISoundSource* endGameSound = engine->addSoundSourceFromFile("../Include/AudioClip/victory_impostor.wav");
	endGameSound->setDefaultVolume(0.2f);
	endGameSound->forceReloadAtNextUse();
	ISoundSource* completeGameSound = engine->addSoundSourceFromFile("../Include/AudioClip/victory_crew.wav");
	completeGameSound->setDefaultVolume(0.2f);
	completeGameSound->forceReloadAtNextUse();
	ISoundSource* ghostKillSound = engine->addSoundSourceFromFile("../Include/AudioClip/impostor_kill.wav");
	ghostKillSound->setDefaultVolume(0.2f);
	ghostKillSound->forceReloadAtNextUse();
	ISoundSource* scanSound = engine->addSoundSourceFromFile("../Include/AudioClip/panel_medbayscan.wav");
	scanSound->setDefaultVolume(0.2f);
	scanSound->forceReloadAtNextUse();
	ISoundSource* ghostSound = engine->addSoundSourceFromFile("../Include/AudioClip/ghost.wav");
	ghostSound->forceReloadAtNextUse();

	ISoundSource* pasos[8];
	cargarSonidoPasos(engine, pasos);
	int ultimoPaso = 0;

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																		FRAME BUFFERS																				 //
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Skybox VAO
	unsigned int skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	// Model FrameBuffers
	unsigned int frameBufferFBO;
	glGenFramebuffers(1, &frameBufferFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBufferFBO);
	// create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
	unsigned int colorBuffers[2];
	glGenTextures(2, colorBuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_W, SCR_H, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// attach texture to framebuffer
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
	}

	// create and attach depth buffer (renderbuffer)
	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_W, SCR_H);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete111!" << std::endl;

	// ping-pong-framebuffer for blurring
	unsigned int pingpongFBO[2];
	unsigned int pingpongColorbuffers[2];
	glGenFramebuffers(2, pingpongFBO);
	glGenTextures(2, pingpongColorbuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
		glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_W, SCR_H, 0, GL_RGBA, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
		// also check if framebuffers are complete (no need for depth buffer)
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
	}

	unsigned int frameBufferMultisampledFBO;
	glGenFramebuffers(1, &frameBufferMultisampledFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBufferMultisampledFBO);

	// create a multisampled color attachment texture
	unsigned int ColorBufferMultiSampled[2];
	glGenTextures(2, ColorBufferMultiSampled);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ColorBufferMultiSampled[i]);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, maxSamples, GL_RGBA, SCR_W, SCR_H, GL_TRUE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D_MULTISAMPLE, ColorBufferMultiSampled[i], 0);
	}

	// create a (also multisampled) renderbuffer object for depth and stencil attachments
	unsigned int rboDepthMultisampled;
	glGenRenderbuffers(1, &rboDepthMultisampled);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepthMultisampled);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, maxSamples, GL_DEPTH24_STENCIL8, SCR_W, SCR_H);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepthMultisampled);

	unsigned int attachmentsMultisampled[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachmentsMultisampled);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete111!" << std::endl;

	// NEW- SHADOWS
	// Framebuffer for Shadow Map
	unsigned int shadowMapFBO;
	glGenFramebuffers(1, &shadowMapFBO);

	// Texture for Shadow Map FBO
	unsigned int shadowMapWidth = 4096, shadowMapHeight = 2880;
	unsigned int shadowMap;
	glGenTextures(1, &shadowMap);
	glBindTexture(GL_TEXTURE_2D, shadowMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	// Prevents darkness outside the frustrum
	float clampColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, clampColor);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
	// Needed since we don't touch the color buffer
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Matrices needed for the light's perspective
	glm::vec3 SunPosition = glm::vec3(26.0f, 40.0f, 35.0f); //glm::vec3(25,20, 45);
	glm::mat4 orthgonalProjection = glm::ortho(-24.0f, 24.0f, -15.0f, 15.0f, 50.0f, 78.0f);
	glm::mat4 lightView = glm::lookAt(SunPosition, glm::vec3(26, -2, -30), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 lightProjection = orthgonalProjection * lightView;

	shadowMapProgram.use();
	glUniformMatrix4fv(glGetUniformLocation(shadowMapProgram.ID, "lightProjection"), 1, GL_FALSE, glm::value_ptr(lightProjection));
	glm::mat4 modelShadow = glm::mat4(1.f);
	shadowMapProgram.setMat4("model", modelShadow);

	ShadowDebug.use();
	ShadowDebug.setInt("shadowMap", 0);


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//																		LOOP PRINCIPAL																				 //
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	int i = 0;
	while (running) // the event loop
	{
		// Frame time logic
		Uint32 currentFrame = SDL_GetTicks();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		timeN = currentFrame / 15;

		// Diff real time - iteration time
		diff = timeN - timeAux;
		timeAux = timeN;

		glClearColor(0.0, 0.0, 0.0, 0.0);					// set background colour
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear window

		switch (actualState)
		{
		case INIT: {
			completedMissions = vector<bool>(5, false); //first three are cables, last two are twoFactor
			//3 unique int. Position equals to position in completedMissions, number equals to pos in CableSpots
			assignedWire = randomUnique(3, cableSpots.size());
			actualState = MAIN_MENU;
			alpha = 1;
			transitionCounter = 0;
			cortoElectricidad = false;
			pausarSonidos(engine);
			engine->stopAllSounds();
			engine->play2D(mainMenuSound, true);
			SDL_ShowCursor(SDL_ENABLE);
			lock_cam = false;
			delete(ghost);
			ghost = new IA();
			camera->setPos(glm::vec3(29.26f, 0.31f, -24.32f));
			camera->setFront(glm::vec3(0.0f, 0.0f, -1.0f));
			old_pos = glm::vec3(0.f);
			old_pos_camera = camera->getPos();
			old_front_camera = camera->getFront();
			se_activa_el_fantasma = false;
			modelFantasma = glm::mat4(1.f);
			modelFantasma = glm::translate(modelFantasma, glm::vec3(20.3f, 0.2f, -12.70f));
			modelFantasma = glm::scale(modelFantasma, glm::vec3(0.15f, 0.15f, 0.15f));
			first_person = false;
			fixed_pos = false;
			soundKill = true;
			medic_Scan = true;
			break;
		}
		case GAME:
		{
			float cameraSpeed = 0.005f * deltaTime; // adjust accordingly

			// Audio processing
			engine->setListenerPosition(vec3df(camera->getPos().x, camera->getPos().y, camera->getPos().z),
				vec3df(-camera->getFront().x, camera->getFront().y, -camera->getFront().z));

			// Render

			// DRAW SKYBOX
			glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content
			glDisable(GL_DEPTH_TEST);
			skyboxShader.use();
			skyboxShader.setInt("sun", false);
			view = glm::mat4(glm::mat3(glm::lookAt(camera->getPos() - camera->getFront(), camera->getPos() + camera->getFront(), camera->getUp()))); // remove translation from the view matrix
			projection = glm::perspective(glm::radians(zoom), (float)SCR_W / (float)SCR_H, 0.5f, 100.f);
			skyboxShader.setMat4("view", view);
			skyboxShader.setMat4("projection", projection);
			model = glm::mat4(1.f);
			skyboxShader.setMat4("model", model);

			// SKYBOX CUBE
			glBindVertexArray(skyboxVAO);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
			glDrawArrays(GL_TRIANGLES, 0, 36);
			glBindVertexArray(0);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS); // set depth function back to default

			// CONFIG LIGHTS
			ourShader.use();

			glUniform1i(glGetUniformLocation(ourShader.ID, "specular_map"), specular_map);

			if (cortoElectricidad)
			{
				sonido = true;
				if (count < 65)
				{
					if (count == 0)
					{
						pausarSonidos(engine);
						engine->play2D(ligthOffSound);
					}
					if (diff != timeN)
					{
						diffuse = glm::vec3(diffuse.x - 0.013f * diff);
						specular = glm::vec3(specular.x - 0.013f * diff);
						diffuseHall = diffuse;
						specularHall = specular;
						count += diff;
					}
				}
				else if (count >= 65 && !apagon)
				{
					diffuse = glm::vec3(0.f);
					specular = diffuse;
					diffuseHall = diffuse;
					specularHall = specular;
					apagon = true;
				}

				if (apagon)
				{
					if (count < 130)
					{
						diffuseHall = glm::vec3(diffuseHall.x + 0.02f * diff, 0.f, 0.f);
						specularHall = glm::vec3(specularHall.x + 0.02f * diff, 0.f, 0.f);
						count += diff;
						if (count >= 85 && count < 85 + diff)
							engine->play2D(sirenSound);
					}
					else if (count < 200)
					{
						diffuseHall = glm::vec3(diffuseHall.x - 0.02f * diff, 0.f, 0.f);
						specularHall = glm::vec3(specularHall.x - 0.02f * diff, 0.f, 0.f);
						count += diff;
					}
					else if (count >= 200)
					{
						diffuseHall = glm::vec3(0.05f, 0.f, 0.f);
						specularHall = diffuseHall;
						count = 65;
					}
				}
			}
			else
			{
				engine->stopAllSoundsOfSoundSource(sirenSound);
				engine->stopAllSoundsOfSoundSource(ligthOffSound);
				diffuse = glm::vec3(0.8f);
				specular = glm::vec3(0.6f);
				diffuseHall = glm::vec3(1.0f, 1.0f, 1.0f);
				specularHall = glm::vec3(1.0f, 1.0f, 1.0f);
				if (sonido)
				{
					iniciarSonidos(engine);
					sonido = false;
				}
				apagon = false;
				count = 0;
			}

			if (!ghost->gameOver() && !cables && !isTwoFactorTask) {
				move(mv, camera, cameraSpeed, ourModel, engine, pasos, ultimoPaso, fixed_pos);
			}

			if (linterna)
			{
				if (fixed_pos)
				{
					ourShader.setVec3("characterLight.position", old_pos_camera);
					ourShader.setVec3("characterLight.direction", old_front_camera);
				}
				else
				{
					ourShader.setVec3("characterLight.position", camera->getPos());
					ourShader.setVec3("characterLight.direction", camera->getFront());
				}
			}
			ourShader.setBool("linterna", linterna);

			ourShader.setBool("apagon", apagon);
			configLightsMap(ourShader, diffuse, specular);
			configLightsHall(ourShader, ambient, diffuseHall, specularHall);
			ourShader.setBool("anim", false);

			view = glm::lookAt(camera->getPos() - camera->getFront(), camera->getPos() + camera->getFront(), camera->getUp());
			projection = glm::perspective(glm::radians(zoom), (float)SCR_W / (float)SCR_H, 0.5f, 100.f);
			ourShader.setMat4("projection", projection);
			ourShader.setMat4("view", view);
			ourShader.setVec3("viewPos", camera->getPos());

			//DRAW SUN
			skyboxShader.use();
			//modelsun = glm::translate(modelsun, glm::vec3(27.0f, 0.0f, -17.0f));
			modelsun = glm::mat4(glm::rotate(modelsun, glm::radians(0.5f), glm::vec3(1.0, 0.0, 0.0)));
			//modelsun = glm::translate(modelsun, glm::vec3(0.0f, -0.8f, 1.0f));
			//modelsun = glm::translate(modelsun, glm::vec3(-27.0f, 0.0f, 17.0f));
			skyboxShader.setMat4("model", modelsun);
			skyboxShader.setMat4("projection", projection);
			skyboxShader.setMat4("view", view);
			skyboxShader.setBool("is_sun", true);

			glDisable(GL_MULTISAMPLE);
			glBindFramebuffer(GL_FRAMEBUFFER, frameBufferFBO);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if (glm::distance(camera->getPos(), glm::vec3(29.26f, 1.5f, -24.32f)) < 100.0f)
			{
				sun.Draw(skyboxShader, false, 0, 0);
			}
			skyboxShader.setBool("is_sun", false);

			bool horizontal = true;
			if (bloom)
			{
				blur.use();
				blur.setMat4("projection", projection);
				blur.setMat4("view", view);
				blur.setMat4("model", modelsun);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, colorBuffers[1]);
				for (unsigned int i = 0; i < amount; i++)
				{
					glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
					//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					blur.setInt("horizontal", horizontal);
					// bind texture of other framebuffer
					sun.Draw(blur, false, 1, 1);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[horizontal]);
					horizontal = !horizontal;
				}
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			bloomFinal.use();
			bloomFinal.setMat4("projection", projection);
			bloomFinal.setMat4("view", view);
			bloomFinal.setMat4("model", modelsun);
			bloomFinal.setInt("bloom", bloom);
			bloomFinal.setFloat("exposure", exposure);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
			sun.Draw(bloomFinal, false, 1, 1);

			// DRAW MODEL	
			// Preparations for the Shadow Map
			glViewport(0, 0, shadowMapWidth, shadowMapHeight);
			glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
			glClear(GL_DEPTH_BUFFER_BIT);

			// Draw scene for shadow map
			shadowMapProgram.use();
			ourModel.Draw(shadowMapProgram, false, 1, 1);
			glViewport(0, 0, SCR_W, SCR_H);

			ourShader.use();
			glUniformMatrix4fv(glGetUniformLocation(ourShader.ID, "lightProjection"), 1, GL_FALSE, glm::value_ptr(lightProjection));
			model = glm::mat4(1.0f);
			ourShader.setMat4("model", model);
			ourShader.setBool("shadowMapEnabled", shadowMapEnabled);
			glActiveTexture(GL_TEXTURE13);
			glBindTexture(GL_TEXTURE_2D, shadowMap);
			glUniform1i(glGetUniformLocation(ourShader.ID, "shadowMap"), 13);
			ourModel.Draw(ourShader, false, 0, 0);

			glDisable(GL_MULTISAMPLE);
			glBindFramebuffer(GL_FRAMEBUFFER, frameBufferFBO);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if (glm::distance(camera->getPos(), glm::vec3(29.26f, 1.5f, -24.32f)) < 100.0f)
			{
				ourModel.Draw(ourShader, false, 0, 0);
			}

			if (antialiasing)
			{
				glEnable(GL_MULTISAMPLE);
				glBindFramebuffer(GL_FRAMEBUFFER, frameBufferMultisampledFBO);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				if (glm::distance(camera->getPos(), glm::vec3(29.26f, 1.5f, -24.32f)) < 100.0f)
				{
					ourModel.Draw(ourShader, false, 0, 0);
				}

				glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBufferMultisampledFBO);
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, frameBufferFBO);
				glReadBuffer(GL_COLOR_ATTACHMENT0);
				glDrawBuffer(GL_COLOR_ATTACHMENT0);
				glClear(GL_COLOR_BUFFER_BIT);
				glBlitFramebuffer(0, 0, SCR_W, SCR_H, 0, 0, SCR_W, SCR_H, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
				glReadBuffer(GL_COLOR_ATTACHMENT1);
				glDrawBuffer(GL_COLOR_ATTACHMENT1);
				glClear(GL_COLOR_BUFFER_BIT);
				glBlitFramebuffer(0, 0, SCR_W, SCR_H, 0, 0, SCR_W, SCR_H, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
				glDrawBuffers(2, attachments);
			}

			horizontal = true;
			if (bloom)
			{
				blur.use();
				blur.setMat4("projection", projection);
				blur.setMat4("view", view);
				blur.setMat4("model", model);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, colorBuffers[1]);
				for (unsigned int i = 0; i < amount; i++)
				{
					glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
					//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
					blur.setInt("horizontal", horizontal);
					// bind texture of other framebuffer
					ourModel.Draw(blur, false, 1, 1);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[horizontal]);
					horizontal = !horizontal;
				}
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			bloomFinal.use();
			bloomFinal.setMat4("projection", projection);
			bloomFinal.setMat4("view", view);
			bloomFinal.setMat4("model", model);
			bloomFinal.setInt("bloom", bloom);
			bloomFinal.setFloat("exposure", exposure);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, colorBuffers[0]);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
			ourModel.Draw(bloomFinal, false, 1, 1);

			//DRAW DEL CADAVER
			ourShader.use();
			ourShader.setBool("anim", true);
			ourShader.setBool("moove", false);
			ourShader.setMat4("model", modelCadaver);
			ourShader.setMat4("normals_matrix", matr_normals);
			muerto.Draw(ourShader, false, 0, 0);


			//DRAW DEL FANTASMA
			if ((glm::distance(old_pos_camera.x, 18.f) <= 1.0f) && (glm::distance(old_pos_camera.z, -6.0f) <= 1.0f))
			{ // If se encuentra en la puerta de electricidad
				se_activa_el_fantasma = true;
			}

			if (!ghost->gameOver())
			{
				if (!ghost->isActive() && se_activa_el_fantasma)
				{
					ghost->start(old_pos_camera, diff);
				}
				if (ghost->isActive())
				{
					ghost->setSpeed(diff);
					ghost->update(old_pos_camera);
					if (distance(old_pos_camera, ghost->getPos()) < 5) {
						if (!engine->isCurrentlyPlaying(ghostSound)) {
							engine->play3D(ghostSound, vec3df(ghost->getPos().x, ghost->getPos().y, ghost->getPos().z), false, false, true);
						}
					}

				}
				modelFantasma = glm::mat4(1.0f);
				modelFantasma = glm::translate(modelFantasma, glm::vec3(ghost->getPos().x, 0.f, ghost->getPos().z));

				rotatematrix = glm::mat4(1.0f);
				float angle = glm::acos(glm::dot(glm::normalize(ghost->getDirection()), glm::vec3(0.0, 0.0, 1.0)));
				if (ghost->getDirection().x < 0)
				{
					angle = -angle;
				}
				rotatematrix = glm::mat4(glm::rotate(rotatematrix, angle, glm::vec3(0.0, 1.0, 0.0)));

				modelFantasma = modelFantasma * rotatematrix;
				modelFantasma = glm::scale(modelFantasma, glm::vec3(0.15f, 0.15f, 0.15f));
			}


			ourShader.setMat4("model", modelFantasma);
			ourShader.setBool("moove", true);
			fantasma.initBonesForShader(ourShader);
			fantasma.Draw(ourShader, true, 0, 0);

			//DRAW DEL ASTRONAUTA
			modelAnim = glm::mat4(1.f);
			if (medic_Scan_available && glm::distance(camera->getPos(), glm::vec3(20.7, 0.0, -15.8)) <= 1.0f) {
				//renderF(ShadowDebug, use);
				if (medic_Scan) {
					if (soundScan) {
						engine->play2D(scanSound);
						soundScan = false;
					}
					if (engine->isCurrentlyPlaying(scanSound)) {
						glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
						modelAnim = glm::translate(modelAnim, glm::vec3(20.7, 0.2, -15.8));
						medicRot = glm::rotate(medicRot, 0.01f * diff, glm::vec3(0.0, 1.0, 0.0));
						//fixed_pos = true;
						modelAnim = modelAnim * medicRot;
					}
					else {
						medic_Scan_available = false;
						//fixed_pos = false;
					}

				}
			}
			else {
			
				if (first_person)
				{
					glDisable(GL_CULL_FACE);
					modelAnim = glm::translate(modelAnim, camera->getPos() - camera->getDirection());
					modelAnim = glm::rotate(modelAnim, glm::radians(-yaw + 90), glm::vec3(0.0, 1.0, 0.0));
					modelAnim = glm::rotate(modelAnim, glm::radians(pitch), glm::vec3(-1.0, 0.0, 0.0));
					modelAnim = glm::translate(modelAnim, -glm::vec3(0.f, 0.25f, -0.06f));
				}
				else
				{
					if (fixed_pos)
					{
						modelAnim = glm::translate(modelAnim, old_pos);
						modelAnim = glm::rotate(modelAnim, glm::radians(old_yaw), glm::vec3(0.0, 1.0, 0.0));
					}
					else
					{
						modelAnim = glm::translate(modelAnim, glm::vec3(camera->getPos().x, camera->getPos().y - 0.3, camera->getPos().z));
						modelAnim = glm::rotate(modelAnim, glm::radians(-yaw + 90), glm::vec3(0.0, 1.0, 0.0));
						old_yaw = -yaw + 90;
						old_pos = glm::vec3(camera->getPos().x, camera->getPos().y - 0.3, camera->getPos().z);
						old_pos_camera = camera->getPos();
						old_front_camera = camera->getFront();
					}
				}

			} //aca el else del wire

			if (!(mv.moving_forward || mv.moving_back) || first_person)
			{
				modelAnim = glm::mat4(glm::rotate(modelAnim, glm::radians(-90.0f), glm::vec3(1.0, 0.0, 0.0)));
			}


			modelAnim = glm::scale(modelAnim, glm::vec3(0.13f));
			ourShader.setMat4("model", modelAnim);

			glm::mat4 matr_normals_cube = glm::mat4(glm::transpose(glm::inverse(modelAnim)));
			ourShader.setMat4("normals_matrix", matr_normals_cube);
			ourShader.setBool("anim", true);
			ourShader.setBool("moove", !first_person && (mv.moving_forward || mv.moving_back));
			cuerpo1.initBonesForShader(ourShader);
			cuerpo1.Draw(ourShader, !first_person && (mv.moving_forward || mv.moving_back), 0, 0);

			glEnable(GL_CULL_FACE); // enable back face culling - try this and see what happens!
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);


			if (!ghost->gameOver()) {
				// DRAW DEL MAPA
				ShadowDebug.use();
				ShadowDebug.setBool("transparencyIsAvailable", false);
				if (renderMap && !first_person)
				{
					float delta_x = (10.0f / (SCR_W / 2.0f));
					float delta_y = (10.0f / (SCR_H / 2.0f));
					float mark_x, mark_y;
					if (renderMapComplete) {
						mark_x = (camera->getPos().x * (1.0f / 30.0f)) - 0.9;//+ 0.045333f
						mark_y = -(camera->getPos().z * (1.0f / 20.0f) + 0.7);//- 0.13667f)
					}
					else {
						mark_x = (camera->getPos().x * (1.0f / 60.0f)) + 0.045333f;
						mark_y = -(camera->getPos().z * (1.0f / 40.0f) - 0.13667f);
					}

					float marker[] = {
						// positions        // texture Coords
						mark_x, mark_y + delta_y, 0.0f, 0.0f, 1.0f,
						mark_x, mark_y, 0.0f, 0.0f, 0.0f,
						mark_x + delta_x, mark_y + delta_y, 0.0f, 1.0f, 1.0f,
						mark_x + delta_x, mark_y, 0.0f, 1.0f, 0.0f,
					};
					glBindFramebuffer(GL_FRAMEBUFFER, 0);
					glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content
					glDisable(GL_DEPTH_TEST);
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, gameTexture);
					if (!renderMapComplete)
					{
						renderQuad(upRight);
						glActiveTexture(GL_TEXTURE1);
						glBindTexture(GL_TEXTURE_2D, mapMarker);
						renderQuad(marker);
					}
					else
					{
						renderQuad(full);
						glActiveTexture(GL_TEXTURE1);
						glBindTexture(GL_TEXTURE_2D, mapMarker);
						renderQuad(marker);
						//render cable task markers
						for (int i = 0; i < assignedWire.size(); i++) {
							if (!completedMissions[i]) {
								glm::vec2 markerPos = getScaledCoords(cableSpotsMap[assignedWire[i]] - glm::vec2(10, 10));
								glm::vec2 markerPosFinal = getScaledCoords(cableSpotsMap[assignedWire[i]] + glm::vec2(15, 15));
								renderQuad(ShadowDebug, markerPos, markerPosFinal, icon_error);
							}
						}
						//render twoFactor task markers
						for (int i = 0; i < 2; i++) {
							if (!completedMissions[i + 3]) {
								glm::vec2 markerPos = getScaledCoords(oxygenSpotsMap[i] - glm::vec2(10, 10));
								glm::vec2 markerPosFinal = getScaledCoords(oxygenSpotsMap[i] + glm::vec2(15, 15));
								renderQuad(ShadowDebug, markerPos, markerPosFinal, icon_error);
							}
						}
					}
				}

			}
			else {
				if (soundKill) {
					engine->play2D(ghostKillSound);
					soundKill = false;
				}
				if (wasted <= 1.0) {
					wasted += 0.1;
					renderQuad(ShadowDebug, glm::vec2(1 - 2 * wasted, wasted - 0.4f), glm::vec2(2 - wasted, -wasted + 0.4f), deadTexture);
				}
				renderQuad(ShadowDebug, glm::vec2(-2 + wasted, wasted - 0.4f), glm::vec2(wasted, -wasted + 0.4f), deadTexture);
				if (transitionCounter <= 240) {
					transitionCounter += diff;
				}
				else {
					transitionCounter = 0;
					alpha = 0;
					engine->play2D(endGameSound);
					actualState = END_GAME;
				}
			}

			// SHOW FPS
			short fps_to_show;
			i++;
			if (i == 60)
			{
				i = 0;
				fps_to_show = round(1000.0f / deltaTime);
			}

			string fps = "FPS: " + to_string(fps_to_show);
			SDL_Surface* surf = TTF_RenderText_Blended(gameFont, fps.c_str(), text_color);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content
			glDisable(GL_DEPTH_TEST);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surf->pixels);
			renderQuad(up_left);
			SDL_FreeSurface(surf);

			// Transition from main menu
			if (previousState == TRANSITION)
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glDepthFunc(GL_LEQUAL);
				glDisable(GL_DEPTH_TEST);
				ShadowDebug.use();
				ShadowDebug.setBool("transparencyIsAvailable", true);
				if (transitionCounter <= 130)
				{
					transitionCounter += diff;
					alpha -= 0.01;
					if (alpha <= 0) { alpha = 0; }


					ShadowDebug.setFloat("alpha", alpha);
				}
				else
				{
					previousState = GAME;
				}
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, blackWindows);
				renderQuad(full);
			}


			// Wires Task
			currentMission = closestIndex(camera->getPos(), cableSpots, 0.5f);
			missionIndex = getIndex(assignedWire, currentMission);
			if (missionIndex != -1 && !completedMissions[missionIndex]) {
				renderF(ShadowDebug, use);
			}
			if (cables && missionIndex != -1 && !completedMissions[missionIndex])
			{
				playCables(ShadowDebug, getScaledCoords(last_click), getScaledCoords(mouse_pos), btn_down, reset, engine, wireSound, diff);

				reset = false;
				bool wiresComplete = true;
				for (int cable = 0; cable < 4; cable++)
				{
					wiresComplete = wiresComplete && fixed_cables[cable];
				}
				if (wiresComplete) {
					if (timeVisibleState < 80) {
						if (soundTaskComplete)
						{
							soundTaskComplete = false;
							engine->play2D(taskCompleteSound);
						}

						string taskComplete = "Tarea Completada!";
						SDL_Surface* bg_surface = TTF_RenderText_Blended(gameFontOutline, taskComplete.c_str(), black_color);
						SDL_Surface* fg_surface = TTF_RenderText_Blended(gameFont, taskComplete.c_str(), white_color);
						SDL_Rect rect = { 6, 6, fg_surface->w, fg_surface->h };

						/* blit text onto its outline */
						SDL_SetSurfaceBlendMode(fg_surface, SDL_BLENDMODE_BLEND);
						SDL_BlitSurface(fg_surface, NULL, bg_surface, &rect);
						SDL_FreeSurface(fg_surface);

						glBindTexture(GL_TEXTURE_2D, tex);
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bg_surface->w, bg_surface->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, bg_surface->pixels);
						renderQuad(taskCompletePosition);
						SDL_FreeSurface(bg_surface);
						timeVisibleState += diff;
					}
					else {
						engine->play2D(panelDisappearSound);
						renderMap = true;
						cables = false;
						lock_cam = true;
						reset = true;
						soundTaskComplete = true;
						SDL_ShowCursor(SDL_DISABLE);
						completedMissions[missionIndex] = true;
					}
				}
			}

			// Two Factor Task
			twoFactorMission = closestIndex(camera->getPos(), oxygenSpots, 0.5f);
			if (twoFactorMission != -1 && !completedMissions[twoFactorMission + 3])
			{
				renderF(ShadowDebug, use);
			}
			if (isTwoFactorTask && twoFactorMission != -1 && !completedMissions[twoFactorMission + 3])
			{
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glDepthFunc(GL_LEQUAL);
				glDisable(GL_DEPTH_TEST);
				ShadowDebug.use();
				ShadowDebug.setBool("transparencyIsAvailable", false);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, twoFactorBase);
				renderQuad(full);

				string digits = "   ";
				string pass = "";
				for (int num = 0; num < 5; num++)
				{
					char digit = twoFactorNumbers[num];
					digits += digit;
					if (digit != ' ')
					{
						pass += digit;
					}
				}
				digits += "   ";

				SDL_Surface* surfTwoFactor = TTF_RenderText_Blended(digitalFont, digits.c_str(), white_color);
				glBindTexture(GL_TEXTURE_2D, tex);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surfTwoFactor->w, surfTwoFactor->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surfTwoFactor->pixels);
				renderQuad(twoFactorPosition);
				SDL_FreeSurface(surfTwoFactor);

				// render pass
				surfTwoFactor = TTF_RenderText_Blended(font, twoFactorPass.c_str(), white_color);
				glBindTexture(GL_TEXTURE_2D, tex);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surfTwoFactor->w, surfTwoFactor->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surfTwoFactor->pixels);
				renderQuad(twoFactorPositionPass);
				SDL_FreeSurface(surfTwoFactor);

				if (confirmPass)
				{
					string errorStatus = "   ERROR   ";
					string okStatus = "    OK     ";
					if (pass == twoFactorPass)
					{
						surfTwoFactor = TTF_RenderText_Blended(digitalFont, okStatus.c_str(), white_color);
						if (timeVisibleState < 20)
						{
							if (soundConfirmTask)
							{
								soundConfirmTask = false;
								engine->play2D(enterIdGoodSound);
							}
							timeVisibleState += diff;
						}
						else if (timeVisibleState < 100) {
							if (soundTaskComplete)
							{
								soundTaskComplete = false;
								engine->play2D(taskCompleteSound);
							}

							string taskComplete = "Tarea Completada!";
							SDL_Surface* bg_surface = TTF_RenderText_Blended(gameFontOutline, taskComplete.c_str(), black_color);
							SDL_Surface* fg_surface = TTF_RenderText_Blended(gameFont, taskComplete.c_str(), white_color);
							SDL_Rect rect = { 6, 6, fg_surface->w, fg_surface->h };

							/* blit text onto its outline */
							SDL_SetSurfaceBlendMode(fg_surface, SDL_BLENDMODE_BLEND);
							SDL_BlitSurface(fg_surface, NULL, bg_surface, &rect);
							SDL_FreeSurface(fg_surface);

							glBindTexture(GL_TEXTURE_2D, tex);
							glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bg_surface->w, bg_surface->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, bg_surface->pixels);
							renderQuad(taskCompletePosition);
							SDL_FreeSurface(bg_surface);
							timeVisibleState += diff;
						}
						else
						{
							engine->play2D(panelDisappearSound);
							renderMap = true;
							isTwoFactorTask = false;
							lock_cam = true;
							soundConfirmTask = true;
							soundTaskComplete = true;
							SDL_ShowCursor(SDL_DISABLE);
							completedMissions[twoFactorMission + 3] = true;
						}
					}
					else
					{
						surfTwoFactor = TTF_RenderText_Blended(digitalFont, errorStatus.c_str(), white_color);
						if (timeVisibleState < 40)
						{
							if (soundConfirmTask)
							{
								soundConfirmTask = false;
								engine->play2D(enterIdBadSound);
							}
							timeVisibleState += diff;
						}
						else
						{
							confirmPass = false;
							soundConfirmTask = true;
							timeVisibleState = 0;
							for (int num = 0; num < 5; num++)
							{
								twoFactorNumbers[num] = ' ';
							}
						}
					}
					glBindTexture(GL_TEXTURE_2D, tex);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surfTwoFactor->w, surfTwoFactor->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surfTwoFactor->pixels);
					renderQuad(twoFactorStatusPosition);
					SDL_FreeSurface(surfTwoFactor);
				}
			}

			// Options Menu
			if (optionsMenu) {
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glDepthFunc(GL_LEQUAL);
				glDisable(GL_DEPTH_TEST);
				ShadowDebug.use();
				ShadowDebug.setBool("transparencyIsAvailable", false);
				ShadowDebug.setFloat("alpha", 0.5f);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, windowsMenuOptions);
				renderQuad(optionMenuPositions);

				ShadowDebug.setBool("transparencyIsAvailable", false);
				string optionMenu = "Menú De Opciones";
				SDL_Surface* bg_surface = TTF_RenderText_Blended(gameFontOutline, optionMenu.c_str(), black_color);
				SDL_Surface* fg_surface = TTF_RenderText_Blended(gameFont, optionMenu.c_str(), white_color);
				SDL_Rect rect = { 6, 6, fg_surface->w, fg_surface->h };

				/* blit text onto its outline */
				SDL_SetSurfaceBlendMode(fg_surface, SDL_BLENDMODE_BLEND);
				SDL_BlitSurface(fg_surface, NULL, bg_surface, &rect);
				SDL_FreeSurface(fg_surface);

				glBindTexture(GL_TEXTURE_2D, tex);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bg_surface->w, bg_surface->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, bg_surface->pixels);
				renderQuad(optionMenuTitlePositions);
				SDL_FreeSurface(bg_surface);

				// Render Options 
				string option;
				SDL_Color color = white_color;
				for (int i = 0; i < 6; i++) {
					color = white_color;
					float positionAux[20];
					switch (i)
					{
					case 0: {
						if (antialiasing) {
							color = green_color;
						}
						option = "Antialiasing";
						memcpy(positionAux, optionOnePositions, sizeof(float) * 20);
						break;
					}
					case 1: {
						if (bloom) {
							color = green_color;
						}
						option = "Bloom          ";
						memcpy(positionAux, optionTwoPositions, sizeof(float) * 20);
						break;
					}
					case 2: {
						if (first_person) {
							color = green_color;
						}
						option = "Primera persona";
						memcpy(positionAux, optionThreePositions, sizeof(float) * 20);
						break;
					}
					case 3: {
						if (specular_map) {
							color = green_color;
						}
						option = "Mapa especular";
						memcpy(positionAux, optionFourPositions, sizeof(float) * 20);
						break;
					}
					case 4: {
						if (fixed_pos) {
							color = green_color;
						}
						option = "Modo libre    ";
						memcpy(positionAux, optionFivePositions, sizeof(float) * 20);
						break;
					}
					case 5: {
						if (shadowMapEnabled) {
							color = green_color;
						}
						option = "Sombras       ";
						memcpy(positionAux, optionSixPositions, sizeof(float) * 20);
						break;
					}
					}

					ShadowDebug.setBool("transparencyIsAvailable", false);
					bg_surface = TTF_RenderText_Blended(gameFontOutline, option.c_str(), black_color);
					fg_surface = TTF_RenderText_Blended(gameFont, option.c_str(), color);
					rect = { 6, 6, fg_surface->w, fg_surface->h };

					/* blit text onto its outline */
					SDL_SetSurfaceBlendMode(fg_surface, SDL_BLENDMODE_BLEND);
					SDL_BlitSurface(fg_surface, NULL, bg_surface, &rect);
					SDL_FreeSurface(fg_surface);

					glBindTexture(GL_TEXTURE_2D, tex);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bg_surface->w, bg_surface->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, bg_surface->pixels);
					renderQuad(positionAux);
					SDL_FreeSurface(bg_surface);
				}
			}

			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS); // set depth function back to

			bool completedGame = true;
			for (int i = 0; i < completedMissions.size(); i++) {
				completedGame &= completedMissions[i];
			}
			if (completedGame) {
				actualState = COMPLETED;
				alpha = 0;
				transitionCounter = 0;
				engine->play2D(completeGameSound);
			}
			break;
		}
		case MAIN_MENU:
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDepthFunc(GL_LEQUAL);
			glDisable(GL_DEPTH_TEST);
			ShadowDebug.use();
			ShadowDebug.setBool("transparencyIsAvailable", false);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, mainMenu);
			renderQuad(full);
			break;
		}
		case TRANSITION:
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDepthFunc(GL_LEQUAL);
			glDisable(GL_DEPTH_TEST);
			ShadowDebug.use();
			ShadowDebug.setBool("transparencyIsAvailable", true);
			if (transitionCounter <= 120)
			{
				if (transitionCounter < 30) {
					if (!engine->isCurrentlyPlaying(roundStartSound)) {
						engine->play2D(roundStartSound);
					}
				}
				transitionCounter += diff;
				alpha -= 0.0003;
				if (alpha <= 0)
				{
					alpha = 0;
				}
				ShadowDebug.setFloat("alpha", alpha);
			}
			else
			{
				engine->play2D(playerSpawnSound);
				actualState = GAME;
				previousState = TRANSITION;
				alpha = 1.0;
				transitionCounter = 0;
			}
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, mainMenu);
			renderQuad(full);
			break;
		}
		case COMPLETED:
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDepthFunc(GL_LEQUAL);
			glDisable(GL_DEPTH_TEST);
			ShadowDebug.use();
			ShadowDebug.setBool("transparencyIsAvailable", true);
			if (engine->isCurrentlyPlaying(completeGameSound))
			{
				alpha += 0.00005;
				if (alpha >= 1)
				{
					alpha = 1;
				}
				ShadowDebug.setFloat("alpha", alpha);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, game_victory);
				renderQuad(ShadowDebug, glm::vec2(-1.f, 1.0f), glm::vec2(1.f, -1.0), game_victory);
			}
			else {
				actualState = INIT;
			}
			break;
		}
		case END_GAME:
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDepthFunc(GL_LEQUAL);
			glDisable(GL_DEPTH_TEST);
			ShadowDebug.use();
			ShadowDebug.setBool("transparencyIsAvailable", true);
			if (engine->isCurrentlyPlaying(endGameSound))
			{
				alpha += 0.00005;
				if (alpha >= 1)
				{
					alpha = 1;
				}
				ShadowDebug.setFloat("alpha", alpha);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, game_over);
				renderQuad(ShadowDebug, glm::vec2(-1.f, 1.0f), glm::vec2(1.f, -1.0), game_over);
			}
			else {
				actualState = INIT;
			}
			break;
		}
		}

		// EVENTS
		while (SDL_PollEvent(&sdlEvent))
		{
			switch (sdlEvent.type)
			{
			case SDL_MOUSEMOTION:
			{
				int x, y;
				SDL_GetMouseState(&x, &y);
				if (lock_cam)
				{
					int xoffset = x - SCR_W / 2;
					int yoffset = SCR_H / 2 - y; // reversed since y-coordinates range from bottom to top

					xoffset *= sensitivity;
					yoffset *= sensitivity;
					yaw += xoffset;
					pitch += yoffset;

					if (pitch > 89.0f)
						pitch = 89.0f;
					if (pitch < -89.0f)
						pitch = -89.0f;

					camera->Rotate(yaw, pitch);
				}
				if (cables)
				{
					mouse_pos = glm::vec2(x, y);
				}
				break;
			}
			case SDL_MOUSEBUTTONDOWN:
			{
				if (sdlEvent.button.button == SDL_BUTTON_LEFT)
				{
					btn_down = true;
					int x, y;
					SDL_GetMouseState(&x, &y);
					if (actualState == MAIN_MENU && isInside(x, y))
					{
						engine->play2D(uiSelectSound);
						actualState = TRANSITION;
						transitionCounter = 0;
						lock_cam = true;
						SDL_ShowCursor(SDL_DISABLE);
						engine->stopAllSoundsOfSoundSource(mainMenuSound);
						iniciarSonidos(engine);
					}
					else if (actualState == GAME)
					{
						if (optionsMenu) {
							int optionClicked = getOptionClicked(x, y);
							switch (optionClicked) {
							case 1: {
								engine->play2D(optionClickedSound);
								antialiasing = !antialiasing;
								break;
							}
							case 2: {
								engine->play2D(optionClickedSound);
								bloom = !bloom;
								break;
							}
							case 3: {
								engine->play2D(optionClickedSound);
								first_person = !first_person;
								break;
							}
							case 4: {
								engine->play2D(optionClickedSound);
								specular_map = !specular_map;
								break;
							}
							case 5: {
								engine->play2D(optionClickedSound);
								fixed_pos = !fixed_pos;
								if (!fixed_pos) {
									camera->setPos(old_pos_camera);
								}
								break;
							}
							case 6: {
								engine->play2D(optionClickedSound);
								shadowMapEnabled = !shadowMapEnabled;
								break;
							}
							}
						}
						if (isTwoFactorTask) {
							char digit = getDigit(x, y);
							if (digit != ' ')
							{
								engine->play2D(enterIdSound);
								for (int num = 0; num < 5; num++)
								{
									if (twoFactorNumbers[num] == ' ')
									{
										twoFactorNumbers[num] = digit;
										break;
									}
								}
							}
							else
							{
								if (getErasePass(x, y))
								{
									engine->play2D(enterIdSound);
									for (int num = 0; num < 5; num++)
									{
										twoFactorNumbers[num] = ' ';
									}
								}
								else
								{
									confirmPass = getConfirmPass(x, y);
								}
							}
						}
						if (cables)
						{
							last_click = glm::vec2(x, y);
						}
					}
					cout << "X = " << x << " Y = " << y << endl;
					cout << "RX = " << getScaledCoords(x, y).x << "RY = " << getScaledCoords(x, y).y << endl;

				}
				break;
			}
			case SDL_MOUSEBUTTONUP:
			{
				btn_down = false;
				break;
			}
			case SDL_MOUSEWHEEL:
			{
				float yoffset = sdlEvent.wheel.y;
				zoom -= (float)yoffset;
				if (zoom < 1.0f)
					zoom = 1.0f;
				if (zoom > 45.0f)
					zoom = 45.0f;
				break;
			}
			case SDL_QUIT:
			{
				running = false;
				break;
			}
			case SDL_KEYUP:
			{
				if (sdlEvent.key.keysym.sym == SDLK_LALT && actualState != MAIN_MENU)
				{
					lock_cam = true;
					SDL_ShowCursor(SDL_DISABLE);
				}
				if (sdlEvent.key.keysym.sym == SDLK_LSHIFT)
				{
					mv.spedUp = false;
				}
				if (sdlEvent.key.keysym.sym == SDLK_w)
				{
					mv.moving_forward = false;
				}
				if (sdlEvent.key.keysym.sym == SDLK_a)
				{
					mv.moving_left = false;
				}
				if (sdlEvent.key.keysym.sym == SDLK_d)
				{
					mv.moving_right = false;
				}
				if (sdlEvent.key.keysym.sym == SDLK_s)
				{
					mv.moving_back = false;
				}
				if (sdlEvent.key.keysym.sym == SDLK_SPACE)
				{
					if (fixed_pos)
						mv.moving_up = false;
				}
				if (sdlEvent.key.keysym.sym == SDLK_LCTRL)
				{
					if (fixed_pos)
						mv.moving_down = false;
				}
				break;
			}
			case SDL_KEYDOWN:
			{
				if (sdlEvent.key.keysym.sym == SDLK_LALT)
				{
					lock_cam = false;
					SDL_ShowCursor(SDL_ENABLE);
				}
				if (sdlEvent.key.keysym.sym == SDLK_LSHIFT)
				{
					mv.spedUp = true;
				}
				if (sdlEvent.key.keysym.sym == SDLK_c)
				{
					cout << "X: " << camera->getPos().x << " Y: " << camera->getPos().y << " Z: " << camera->getPos().z << endl;
					cout << "X: " << camera->getFront().x << " Y: " << camera->getFront().y << " Z: " << camera->getFront().z << endl;
				}
				if (sdlEvent.key.keysym.sym == SDLK_ESCAPE)
				{
					if (actualState == MAIN_MENU)
					{
						running = false;
					}
					else {
						if (isTwoFactorTask || cables || optionsMenu || (renderMapComplete && !first_person))
						{
							engine->play2D(panelDisappearSound);
							renderMap = true;
							renderMapComplete = false;
							isTwoFactorTask = false;
							cables = false;
							optionsMenu = false;
							lock_cam = true;
							SDL_ShowCursor(SDL_DISABLE);
						}
						else
						{
							actualState = INIT;
						}
					}
				}
				if (sdlEvent.key.keysym.sym == SDLK_a)
				{
					mv.moving_left = true;
				}
				if (sdlEvent.key.keysym.sym == SDLK_d)
				{
					mv.moving_right = true;
				}
				if (sdlEvent.key.keysym.sym == SDLK_s)
				{
					if (!ghost->gameOver() && !cables && !isTwoFactorTask) {
						mv.moving_back = true;
					}
				}
				if (sdlEvent.key.keysym.sym == SDLK_w)
				{
					if (!ghost->gameOver() && !cables && !isTwoFactorTask) {
						mv.moving_forward = true;
					}
				}
				if (sdlEvent.key.keysym.sym == SDLK_SPACE)
				{
					if (fixed_pos)
						mv.moving_up = true;
				}
				if (sdlEvent.key.keysym.sym == SDLK_LCTRL)
				{
					if (fixed_pos)
						mv.moving_down = true;
				}
				if (sdlEvent.key.keysym.sym == SDLK_x)
				{
					cortoElectricidad = !cortoElectricidad;
					count = 0;
				}
				if (sdlEvent.key.keysym.sym == SDLK_l)
				{
					linterna = !linterna;
				}
				if (sdlEvent.key.keysym.sym == SDLK_m)
				{
					if (!first_person) {
						engine->play2D((renderMapComplete) ? panelDisappearSound : panelAppearSound);
					}
					renderMapComplete = !renderMapComplete;
				}
				if (sdlEvent.key.keysym.sym == SDLK_2)
				{
					exposure -= 0.02;
				}
				if (sdlEvent.key.keysym.sym == SDLK_3)
				{
					exposure += 0.02;
				}
				if (sdlEvent.key.keysym.sym == SDLK_f)
				{
					medic_Scan = true;
					bool completedWire = missionIndex != -1 && !completedMissions[missionIndex];
					bool completedTwoFactor = twoFactorMission != -1 && !completedMissions[twoFactorMission + 3];
					bool inRangeWires = inRange(camera->getPos(), cableSpots, 0.5f) && !cables && completedWire;
					bool inRangeTwoFactor = inRange(camera->getPos(), oxygenSpots, 0.5f) && !isTwoFactorTask && completedTwoFactor;
					if (inRangeWires || inRangeTwoFactor) {
						timeVisibleState = 0;
						engine->play2D(panelAppearSound);
						renderMap = false;
						lock_cam = false;
						SDL_ShowCursor(SDL_ENABLE);
					}
					if (inRangeWires) {
						cables = true;
					}
					if (inRangeTwoFactor)
					{
						confirmPass = false;
						isTwoFactorTask = true;
						int randomPass = rand() % (98765 - 12345 + 1) + 12345;
						twoFactorPass = to_string(randomPass);
						for (int num = 0; num < 5; num++)
						{
							twoFactorNumbers[num] = ' ';
						}
					}
				}
				if (sdlEvent.key.keysym.sym == SDLK_F11)
				{
					if (fullScreen)
						SDL_SetWindowFullscreen(window, SDL_FALSE);
					else
						SDL_SetWindowFullscreen(window, SDL_TRUE);

					fullScreen = !fullScreen;

					SDL_GetCurrentDisplayMode(0, &DM);
					auto Width = DM.w;
					auto Height = DM.h;
					if (fullScreen)
						glViewport(0, 0, Width, Height);
					else
						glViewport(0, 0, SCR_W, SCR_H);
				}
				if (sdlEvent.key.keysym.sym == SDLK_p) {
					if (!optionsMenu) {
						engine->play2D(panelAppearSound);
						optionsMenu = true;
						lock_cam = false;
						renderMap = false;
						SDL_ShowCursor(SDL_ENABLE);
					}
				}
				break;
			}
			}
			if (lock_cam)
				SDL_WarpMouseInWindow(window, SCR_W / 2, SCR_H / 2);
		}

		SDL_GL_SwapWindow(window);
	}

	cleanup();
	engine->drop();

	// FIN LOOP PRINCIPAL
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
