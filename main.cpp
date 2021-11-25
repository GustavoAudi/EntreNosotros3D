﻿#include <windows.h>
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
#include "Include/ErrorsMSG.h"
#include <irrKlang.h>

#include <iostream>
#include <fstream>
#include "Config/tinyxml2.h"

#include <SDL_ttf.h>
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
GLuint vao, vbo[2]; // handles for our VAO and two VBOs
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

struct movement {
	bool spedUp = false;
	bool moving_forward = false;
	bool moving_back = false;
	bool moving_right = false;
	bool moving_left = false;
	bool moving_up = false;
	bool moving_down = false;
};

void move(movement mv, Camera* camera, float cameraSpeed, Model& ourModel, ISoundEngine*& engine, ISoundSource* pasos[], int& ultimoPaso, bool modoLibre) {
	glm::vec3 cameraPos = camera->getPos();
	glm::vec3 camAux = cameraPos;
	if (mv.spedUp) {
		cameraSpeed = cameraSpeed * 2;
	}
	glm::vec2 direction;
	if (mv.moving_forward || mv.moving_back) {
		direction = glm::normalize(glm::vec2(camera->getFront().x, camera->getFront().z));
	}
	if (mv.moving_forward) {
		cameraPos.x += cameraSpeed * direction.x;
		cameraPos.z += cameraSpeed * direction.y;
		if (!engine->isCurrentlyPlaying(pasos[ultimoPaso]) && !modoLibre) {
			engine->play2D(pasos[(ultimoPaso + 1) % 8]);
			ultimoPaso = (ultimoPaso + 1) % 8;
		}
	}
	if (mv.moving_back) {
		cameraPos.x -= cameraSpeed * direction.x;
		cameraPos.z -= cameraSpeed * direction.y;
		if (!engine->isCurrentlyPlaying(pasos[ultimoPaso]) && !modoLibre) {
			engine->play2D(pasos[(ultimoPaso + 1) % 8]);
			ultimoPaso = (ultimoPaso + 1) % 8;
		}
	}
	if (mv.moving_left) {
		cameraPos -= glm::normalize(glm::cross(camera->getFront(), glm::vec3(0, 1, 0))) * cameraSpeed;
		if (!engine->isCurrentlyPlaying(pasos[ultimoPaso]) && !modoLibre) {
			engine->play2D(pasos[(ultimoPaso + 1) % 8]);
			ultimoPaso = (ultimoPaso + 1) % 8;
		}
	}
	if (mv.moving_right) {
		cameraPos += glm::normalize(glm::cross(camera->getFront(), glm::vec3(0, 1, 0))) * cameraSpeed;
		if (!engine->isCurrentlyPlaying(pasos[ultimoPaso]) && !modoLibre) {
			engine->play2D(pasos[(ultimoPaso + 1) % 8]);
			ultimoPaso = (ultimoPaso + 1) % 8;
		}
	}
	if (mv.moving_up || mv.moving_down) {
		engine->stopAllSoundsOfSoundSource(pasos[ultimoPaso]);
		if (mv.moving_up) {
			cameraPos.y += cameraSpeed;
		}
		if (mv.moving_down) {
			cameraPos.y -= cameraSpeed;
		}
	}
	if (ourModel.MeshCollision2(cameraPos)) {
		cameraPos = camAux;
	}
	camera->setPos(cameraPos);
}

void setupLightsHall(Shader& ourShader) {
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

	while (pLuz != nullptr) {
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

void configLightsHall(Shader& ourShader, glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular) {
	ourShader.setVec3("diffuseSpot", diffuse);
	ourShader.setVec3("specularSpot", specular);
	ourShader.setVec3("ambientSpot", ambient);
}

void setupLightsCharacter(Shader& ourShader) {
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

void setupLightsMap(Shader& ourShader) {
	pLuz = pLucesMapa->FirstChildElement("Luz");

	glm::vec3* posicion = new glm::vec3();
	glm::vec3* ambient = new glm::vec3();
	glm::vec3* diffuse = new glm::vec3();
	glm::vec3* specular = new glm::vec3();
	float* constant = new float();
	float* linear = new float();
	float* quadratic = new float();
	int i = 0;

	while (pLuz != nullptr) {
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

void configLightsMap(Shader& ourShader, glm::vec3 diffuse, glm::vec3 specular) {
	pLuz = pLucesMapa->FirstChildElement("Luz");

	int i = 0;
	while (pLuz != nullptr) {
		string ligthName = "pointLights[" + std::to_string(i);

		ourShader.setVec3(ligthName + "].diffuse", diffuse);
		ourShader.setVec3(ligthName + "].specular", specular);
		if (i == 1 || i == 3 || i == 4) {
			ourShader.setVec3(ligthName + "].diffuse", diffuse / 2.0f);
			ourShader.setVec3(ligthName + "].specular", specular / 2.0f);
		}
		if (i == 15) {
			ourShader.setVec3(ligthName + "].diffuse", diffuse / 3.0f);
			ourShader.setVec3(ligthName + "].specular", specular / 3.0f);
		}

		pLuz = pLuz->NextSiblingElement("Luz");
		i++;
	}
}

void iniciarSonidos(ISoundEngine*& engine) {
	engine->setSoundVolume(0);

	pSonido = pSonidos->FirstChildElement("Sonido");

	glm::vec3* posicion = new glm::vec3();
	float* minVol = new float();
	const char* rut;

	while (pSonido != nullptr) {
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

void pausarSonidos(ISoundEngine*& engine) {

	pSonido = pSonidos->FirstChildElement("Sonido");
	const char* rut;

	while (pSonido != nullptr) {
		rut = pSonido->Attribute("rut");
		engine->stopAllSoundsOfSoundSource(engine->getSoundSource(rut));
		pSonido = pSonido->NextSiblingElement("Sonido");
	}
}

void cargarSonidoPasos(ISoundEngine*& engine, ISoundSource* pasos[8]) {

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

float getOrientation(glm::vec2 A1, glm::vec2 A2, glm::vec2 A3) {
	return (A1.x - A3.x) * (A2.y - A3.y) - (A1.y - A3.y) * (A2.x - A3.x);
}

bool isInside(float x, float y) {
	glm::vec2 P = glm::vec2(x, y);

	glm::vec2 A1 = glm::vec2(445, 567);
	glm::vec2 A2 = glm::vec2(616, 499);
	glm::vec2 A3 = glm::vec2(444, 431);

	bool inside = false;
	float orientationT = getOrientation(A1, A2, A3);
	float orientationT1 = getOrientation(A1, A2, P);
	float orientationT2 = getOrientation(A2, A3, P);
	float orientationT3 = getOrientation(A3, A1, P);
	if (orientationT >= 0) {
		inside = (orientationT1 >= 0) && (orientationT2 >= 0) && (orientationT3 >= 0);
	}
	else {
		inside = (orientationT1 < 0) && (orientationT2 < 0) && (orientationT3 < 0);
	}

	return inside;
}

unsigned int loadTexture(string path) {
	unsigned int texture1;
	// texture 1
	// ---------
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);
	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// set texture wrapping to GL_REPEAT (default wrapping method)
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*																	|							 |
																	|			MAIN			 |
																	|							 |
*//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int main(int argc, char* argv[]) {

	// INITIALIZATION
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
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
	if (maxSamples > 16) maxSamples = 16;
	if (maxSamples < 2) maxSamples = 2;

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

	// Write text to surface
	SDL_Color text_color = { 64, 132, 150 };

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
	glEnable(GL_CULL_FACE); // enable back face culling - try this and see what happens!
	glEnable(GL_DEPTH_CLAMP);
	glEnable(GL_BLEND);
	glDisable(GL_MULTISAMPLE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);// enable depth testing

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
		"../Include/skybox/back.png"
	};

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

	float upRight[] = {
		// positions        // texture Coords
		0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	};

	float full[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	};

	float up_left[] = {
		// positions        // texture Coords
		-1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
		-1.0f, 0.9f, 0.0f, 0.0f, 1.0f,
		-0.9f,  1.0f, 0.0f, 1.0f, 0.0f,
		-0.9f, 0.9f, 0.0f, 1.0f, 1.0f,
	};

	// load textures
	unsigned int cubemapTexture = loadCubemap(faces);
	unsigned int gameTexture = loadTexture("../Include/model/mapa-png.png");
	unsigned int mapMarker = loadTexture("../Include/model/marker.png");
	unsigned int mainMenu = loadTexture("../Include/model/main-menu.png");
	unsigned int blackWindows = loadTexture("../Include/model/black-windows.png");

	// INITIALIZE VARIABLES
	SDL_DisplayMode DM;
	SDL_Event sdlEvent;  // variable to detect SDL events

	// Time tracking
	Uint32 deltaTime = 0;	// Time between current frame and last frame
	Uint32 lastFrame = 0; // Time of last frame

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
	modelFantasma = glm::translate(modelFantasma, glm::vec3(20.3f, 0.3f, -12.70f));
	modelFantasma = glm::scale(modelFantasma, glm::vec3(0.2f, 0.2f, 0.2f));


	glm::mat4 view, model, projection, modelsun;
	glm::mat4 modelAnim = glm::mat4(1.0f);
	modelAnim = glm::translate(modelAnim, glm::vec3(29.26f, 0.0f, -24.32f));
	modelAnim = glm::scale(modelAnim, glm::vec3(0.2f, 0.2f, 0.2f));

	modelsun = glm::mat4(1.0f);
	modelsun = glm::translate(modelsun, glm::vec3(25.0f, 35.0f, 0.0f));//modelsun = glm::translate(modelsun, glm::vec3(25.0f, 35.0f, 35.0f)); //glm::vec3(25.0f, 20.0f, 50.0f));
	modelsun = glm::scale(modelsun, glm::vec3(0.3f, 0.3f, 0.3f));

	enum STATES {
		MAIN_MENU,
		TRANSITION,
		GAME
	};
	STATES actualState = MAIN_MENU;
	STATES previousState = TRANSITION;
	SDL_ShowCursor(SDL_ENABLE);

	bool sonido = false;
	bool running = true;
	bool fullScreen = false;
	bool bloom = true;
	bool antialiasing = true;
	float exposure = 2.0f;
	bool cortoElectricidad = false;
	bool apagon = false;
	bool linterna = false;
	bool specular_map = false;
	bool fixed_pos = false;
	bool first_person = false;
	bool mapa = false;
	float old_yaw = 0.f;
	glm::vec3 old_pos = glm::vec3(0.f);
	glm::vec3 old_pos_camera = camera->getPos();
	glm::vec3 old_front_camera = camera->getFront();
	movement mv;
	bool lock_cam = false;
	bool moved = false;
	int count = 0;
	int timeAux = 0;
	int diff = 0;
	int timeN = 0;

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
	engine->play2D(mainMenuSound);

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

	unsigned int attachmentsMultisampled[2] = { GL_COLOR_ATTACHMENT0 , GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachmentsMultisampled);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer not complete111!" << std::endl;


	//NEW- SHADOWS
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
	while (running)		// the event loop
	{
		// frame time logic
		Uint32 currentFrame = SDL_GetTicks();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		timeN = currentFrame / 15;

		// Diff real time - iteration time
		diff = timeN - timeAux;
		timeAux = timeN;

		glClearColor(0.0, 0.0, 0.0, 0.0); // set background colour
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear window

		switch (actualState) {
		case GAME:
		{
			i++;
			float cameraSpeed = 0.005f * deltaTime; // adjust accordingly

			//audio processing
			engine->setListenerPosition(vec3df(camera->getPos().x, camera->getPos().y, camera->getPos().z),
				vec3df(-camera->getFront().x, camera->getFront().y, -camera->getFront().z));

			//render

			// DRAW SKYBOX
			glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
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
			if (previousState == GAME) {
				if (cortoElectricidad) {
					sonido = true;
					if (count < 65) {
						if (count == 0) {
							pausarSonidos(engine);
							engine->play2D(ligthOffSound);
						}
						if (diff != timeN) {
							diffuse = glm::vec3(diffuse.x - 0.013f * diff);
							specular = glm::vec3(specular.x - 0.013f * diff);
							diffuseHall = diffuse;
							specularHall = specular;
							count += diff;
						}
					}
					else if (count >= 65 && !apagon) {
						diffuse = glm::vec3(0.f);
						specular = diffuse;
						diffuseHall = diffuse;
						specularHall = specular;
						apagon = true;
					}

					if (apagon) {
						if (count < 130) {
							diffuseHall = glm::vec3(diffuseHall.x + 0.02f * diff, 0.f, 0.f);
							specularHall = glm::vec3(specularHall.x + 0.02f * diff, 0.f, 0.f);
							count += diff;
							if (count >= 85 && count < 85 + diff)
								engine->play2D(sirenSound);
						}
						else if (count < 200) {
							diffuseHall = glm::vec3(diffuseHall.x - 0.02f * diff, 0.f, 0.f);
							specularHall = glm::vec3(specularHall.x - 0.02f * diff, 0.f, 0.f);
							count += diff;
						}
						else if (count >= 200) {
							diffuseHall = glm::vec3(0.05f, 0.f, 0.f);
							specularHall = diffuseHall;
							count = 65;
						}

					}
				}
				else {
					engine->stopAllSoundsOfSoundSource(sirenSound);
					engine->stopAllSoundsOfSoundSource(ligthOffSound);
					diffuse = glm::vec3(0.8f);
					specular = glm::vec3(0.6f);
					diffuseHall = glm::vec3(1.0f, 1.0f, 1.0f);
					specularHall = glm::vec3(1.0f, 1.0f, 1.0f);
					if (sonido) {
						iniciarSonidos(engine);
						sonido = false;
					}
					apagon = false;
					count = 0;
				}
			}
			move(mv, camera, cameraSpeed, ourModel, engine, pasos, ultimoPaso, fixed_pos);

			if (linterna) {
				if (fixed_pos) {
					ourShader.setVec3("characterLight.position", old_pos_camera);
					ourShader.setVec3("characterLight.direction", old_front_camera);
				}
				else {
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
			modelsun = glm::translate(modelsun, glm::vec3(27.0f, 0.0f, -17.0f));
			modelsun = glm::mat4(glm::rotate(modelsun, glm::radians(0.5f), glm::vec3(1.0, 0.0, 0.0)));
			//modelsun = glm::translate(modelsun, glm::vec3(0.0f, -0.8f, 1.0f));
			modelsun = glm::translate(modelsun, glm::vec3(-27.0f, 0.0f, 17.0f));
			skyboxShader.setMat4("model", modelsun);
			skyboxShader.setMat4("projection", projection);
			skyboxShader.setMat4("view", view);
			skyboxShader.setBool("is_sun", true);

			glDisable(GL_MULTISAMPLE);
			glBindFramebuffer(GL_FRAMEBUFFER, frameBufferFBO);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if (glm::distance(camera->getPos(), glm::vec3(29.26f, 1.5f, -24.32f)) < 100.0f) {
				sun.Draw(skyboxShader, false, 0, 0);
			}
			skyboxShader.setBool("is_sun", false);

			bool horizontal = true;
			if (bloom) {
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



			// Preparations for the Shadow Map
			glViewport(0, 0, shadowMapWidth, shadowMapHeight);
			glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
			glClear(GL_DEPTH_BUFFER_BIT);

			// Draw scene for shadow map
			shadowMapProgram.use();
			ourModel.Draw(shadowMapProgram, false, 1, 1);

			glViewport(0, 0, SCR_W, SCR_H);


			//DEBUG SHADOW MAP
			//glBindFramebuffer(GL_FRAMEBUFFER, 0);
			/*ShadowDebug.use();
			glActiveTexture(GL_TEXTURE0 + 0);
			glBindTexture(GL_TEXTURE_2D, shadowMap);
			glUniform1i(glGetUniformLocation(ShadowDebug.ID, "shadowMap"), 0);
			renderQuad(); */



			ourShader.use();
			model = glm::mat4(1.0f);
			ourShader.setMat4("model", model);
			glUniformMatrix4fv(glGetUniformLocation(ourShader.ID, "lightProjection"), 1, GL_FALSE, glm::value_ptr(lightProjection));
			glActiveTexture(GL_TEXTURE13);
			glBindTexture(GL_TEXTURE_2D, shadowMap);
			glUniform1i(glGetUniformLocation(ourShader.ID, "shadowMap"), 13);
			ourModel.Draw(ourShader, false, 0, 0);


			glDisable(GL_MULTISAMPLE);
			glBindFramebuffer(GL_FRAMEBUFFER, frameBufferFBO);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			if (glm::distance(camera->getPos(), glm::vec3(29.26f, 1.5f, -24.32f)) < 100.0f) {
				ourModel.Draw(ourShader, false, 0, 0);

			}

			if (antialiasing) {
				glEnable(GL_MULTISAMPLE);
				glBindFramebuffer(GL_FRAMEBUFFER, frameBufferMultisampledFBO);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				if (glm::distance(camera->getPos(), glm::vec3(29.26f, 1.5f, -24.32f)) < 100.0f) {
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
			if (bloom) {
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


			//DRAW DEL ASTRONAUTA
			projection = glm::perspective(glm::radians(zoom), (float)SCR_W / (float)SCR_H, 0.5f, 100.f);
			view = glm::lookAt(camera->getPos(), camera->getPos(), camera->getUp());

			modelAnim = glm::mat4(1.f);


			if (first_person) {
				glDisable(GL_CULL_FACE);
				modelAnim = glm::translate(modelAnim, camera->getPos() - camera->getDirection());
				modelAnim = glm::rotate(modelAnim, glm::radians(-yaw + 90), glm::vec3(0.0, 1.0, 0.0));
				modelAnim = glm::rotate(modelAnim, glm::radians(pitch), glm::vec3(-1.0, 0.0, 0.0));
				modelAnim = glm::translate(modelAnim, -glm::vec3(0.f, 0.25f, -0.06f));
			}
			else {
				if (fixed_pos) {
					modelAnim = glm::translate(modelAnim, old_pos);
					modelAnim = glm::rotate(modelAnim, glm::radians(old_yaw), glm::vec3(0.0, 1.0, 0.0));
				}
				else {
					modelAnim = glm::translate(modelAnim, glm::vec3(camera->getPos().x, camera->getPos().y - 0.3, camera->getPos().z));
					modelAnim = glm::rotate(modelAnim, glm::radians(-yaw + 90), glm::vec3(0.0, 1.0, 0.0));
					old_yaw = -yaw + 90;
					old_pos = glm::vec3(camera->getPos().x, camera->getPos().y - 0.3, camera->getPos().z);
					old_pos_camera = camera->getPos();
					old_front_camera = camera->getFront();
				}
			}

			if (!(mv.moving_forward || mv.moving_back) || first_person) {
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

			//DRAW DEL FANTASMA
			ourShader.setMat4("model", modelFantasma);
			ourShader.setBool("moove", true);
			fantasma.initBonesForShader(ourShader);
			fantasma.Draw(ourShader, true, 0, 0);

			// DRAW DEL MAPA
			float delta_x = (10.0f / (SCR_W / 2.0f));
			float delta_y = (10.0f / (SCR_H / 2.0f));
			float mark_x = (camera->getPos().x * (1.0f / 60.0f)) + 0.045333f;
			float mark_y = -(camera->getPos().z * (1.0f / 40.0f) - 0.13667f);
			float marker[] = {
				// positions        // texture Coords
				mark_x, mark_y + delta_y, 0.0f, 0.0f, 1.0f,
				mark_x, mark_y, 0.0f, 0.0f, 0.0f,
				mark_x + delta_x, mark_y + delta_y, 0.0f, 1.0f, 1.0f,
				mark_x + delta_x, mark_y, 0.0f, 1.0f, 0.0f,
			};
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
			glDisable(GL_DEPTH_TEST);
			ShadowDebug.use();
			ShadowDebug.setBool("transparencyIsAvailable", false);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, gameTexture);
			if (!mapa) {
				renderQuad(upRight);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, mapMarker);
				renderQuad(marker);
			}
			else {
				renderQuad(full);
			}

			// SHOW FPS 
			short fps_to_show;
			if (i == 60) {
				i = 0;
				fps_to_show = round(1000.0f / deltaTime);
			}

			string fps = "FPS: " + to_string(fps_to_show);
			SDL_Surface* surf = TTF_RenderText_Blended(font, fps.c_str(), text_color);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0, GL_BGRA, GL_UNSIGNED_BYTE, surf->pixels);
			renderQuad(up_left);
			SDL_FreeSurface(surf);

			if (previousState == TRANSITION) {
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
				glDisable(GL_DEPTH_TEST);
				ShadowDebug.use();
				ShadowDebug.setBool("transparencyIsAvailable", true);
				if (count <= 150) {
					count += diff;
					alpha -= 0.008;
					if (alpha <= 0) {
						alpha = 0;
					}
					ShadowDebug.setFloat("alpha", alpha);
				}
				else {
					previousState = GAME;
					alpha = 1.0;
					count = 0;
				}
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, blackWindows);
				renderQuad(full);
			}

			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS); // set depth function back to 

			break;
		}
		case MAIN_MENU:
		{
			// DRAW MAIN MENU
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
			glDisable(GL_DEPTH_TEST);
			ShadowDebug.use();
			ShadowDebug.setBool("transparencyIsAvailable", false);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, mainMenu);
			renderQuad(full);
			break;
		}
		case TRANSITION: {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
			glDisable(GL_DEPTH_TEST);
			ShadowDebug.use();
			ShadowDebug.setBool("transparencyIsAvailable", true);
			if (count <= 100) {
				count += diff;
				alpha -= 0.0003;
				if (alpha <= 0) {
					alpha = 0;
				}
				ShadowDebug.setFloat("alpha", alpha);
			}
			else {
				engine->play2D(playerSpawnSound);
				actualState = GAME;
				previousState = TRANSITION;
				alpha = 1.0;
				count = 0;
			}
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, mainMenu);
			renderQuad(full);
			break;
		}
		}
		// EVENTS
		while (SDL_PollEvent(&sdlEvent)) {   //usar SDL_WaitEvent?
			switch (sdlEvent.type) {
			case SDL_MOUSEMOTION: {
				if (lock_cam) {
					int x, y;
					SDL_GetMouseState(&x, &y);
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
				break;
			}
			case SDL_MOUSEBUTTONDOWN: {
				if (sdlEvent.button.button == SDL_BUTTON_LEFT) {
					int x, y;
					SDL_GetMouseState(&x, &y);
					if (actualState == MAIN_MENU && isInside(x, y)) {
						engine->play2D(uiSelectSound);
						actualState = TRANSITION;
						count = 0;
						lock_cam = true;
						SDL_ShowCursor(SDL_DISABLE);
						engine->stopAllSoundsOfSoundSource(mainMenuSound);
						iniciarSonidos(engine);
					}
					cout << "X = " << x << " Y = " << y << endl;
				}
				break;
			}
			case SDL_MOUSEWHEEL: {
				float yoffset = sdlEvent.wheel.y;
				zoom -= (float)yoffset;
				if (zoom < 1.0f)
					zoom = 1.0f;
				if (zoom > 45.0f)
					zoom = 45.0f;
				break;
			}
			case SDL_QUIT: {
				running = false;
				break;
			}
			case SDL_KEYUP: {
				if (sdlEvent.key.keysym.sym == SDLK_LALT && actualState != MAIN_MENU) {
					lock_cam = true;
					SDL_ShowCursor(SDL_DISABLE);
				}
				if (sdlEvent.key.keysym.sym == SDLK_LSHIFT) {
					mv.spedUp = false;
				}
				if (sdlEvent.key.keysym.sym == SDLK_w) {
					mv.moving_forward = false;
				}
				if (sdlEvent.key.keysym.sym == SDLK_a) {
					mv.moving_left = false;
				}
				if (sdlEvent.key.keysym.sym == SDLK_d) {
					mv.moving_right = false;
				}
				if (sdlEvent.key.keysym.sym == SDLK_s) {
					mv.moving_back = false;
				}
				if (sdlEvent.key.keysym.sym == SDLK_SPACE) {
					if (fixed_pos)
						mv.moving_up = false;
				}
				if (sdlEvent.key.keysym.sym == SDLK_LCTRL) {
					if (fixed_pos)
						mv.moving_down = false;
				}
				break;
			}
			case SDL_KEYDOWN: {
				if (sdlEvent.key.keysym.sym == SDLK_LALT) {
					lock_cam = false;
					SDL_ShowCursor(SDL_ENABLE);
				}
				if (sdlEvent.key.keysym.sym == SDLK_LSHIFT) {
					mv.spedUp = true;
				}
				if (sdlEvent.key.keysym.sym == SDLK_c) {
					cout << "X: " << camera->getPos().x << " Y: " << camera->getPos().y << " Z: " << camera->getPos().z << endl;
					cout << "X: " << camera->getFront().x << " Y: " << camera->getFront().y << " Z: " << camera->getFront().z << endl;
				}
				if (sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
					if (actualState == MAIN_MENU) {
						running = false;
					}
					else {
						actualState = MAIN_MENU;
						alpha = 1;
						count = 0;
						cortoElectricidad = false;
						pausarSonidos(engine);
						engine->play2D(mainMenuSound);
						SDL_ShowCursor(SDL_ENABLE);
						lock_cam = false;
					}
				}
				if (sdlEvent.key.keysym.sym == SDLK_a) {
					mv.moving_left = true;
				}
				if (sdlEvent.key.keysym.sym == SDLK_d) {
					mv.moving_right = true;
				}
				if (sdlEvent.key.keysym.sym == SDLK_s) {
					mv.moving_back = true;
				}
				if (sdlEvent.key.keysym.sym == SDLK_w) {
					mv.moving_forward = true;
				}
				if (sdlEvent.key.keysym.sym == SDLK_SPACE) {
					if (fixed_pos)
						mv.moving_up = true;
				}
				if (sdlEvent.key.keysym.sym == SDLK_LCTRL) {
					if (fixed_pos)
						mv.moving_down = true;
				}
				if (sdlEvent.key.keysym.sym == SDLK_x) {
					cortoElectricidad = !cortoElectricidad;
					count = 0;
				}
				if (sdlEvent.key.keysym.sym == SDLK_e) {
					if (camera->getPos().x > 33.1f && camera->getPos().x < 33.5f && camera->getPos().z < -24.0f && camera->getPos().z > -24.9f)
						if (camera->getFront().x > 0.5f && camera->getFront().x < 0.7f && camera->getFront().z < -0.6f && camera->getFront().z > -0.8f)
							cortoElectricidad = !cortoElectricidad;
				}
				if (sdlEvent.key.keysym.sym == SDLK_l) {
					linterna = !linterna;
				}if (sdlEvent.key.keysym.sym == SDLK_b) {
					bloom = !bloom;
				}
				if (sdlEvent.key.keysym.sym == SDLK_m) {
					mapa = !mapa;
				}
				if (sdlEvent.key.keysym.sym == SDLK_v) {
					antialiasing = !antialiasing;
				}
				if (sdlEvent.key.keysym.sym == SDLK_1) {
					specular_map = !specular_map;
				}
				if (sdlEvent.key.keysym.sym == SDLK_2) {
					exposure -= 0.02;
				}
				if (sdlEvent.key.keysym.sym == SDLK_3) {
					exposure += 0.02;
				}
				if (sdlEvent.key.keysym.sym == SDLK_4) {
					first_person = !first_person;
				}
				if (sdlEvent.key.keysym.sym == SDLK_TAB) {
					fixed_pos = !fixed_pos;
					if (!fixed_pos)
						camera->setPos(old_pos_camera);
				}
				if (sdlEvent.key.keysym.sym == SDLK_F11) {
					if (fullScreen)
						SDL_SetWindowFullscreen(window, SDL_FALSE);
					else
						SDL_SetWindowFullscreen(window, SDL_TRUE);

					fullScreen = !fullScreen;

					SDL_GetCurrentDisplayMode(0, &DM);
					auto Width = DM.w;
					auto Height = DM.h;
					if (fullScreen)
						glViewport(0, 0, Width, Height); //update viewport
					else
						glViewport(0, 0, SCR_W, SCR_H);
				}
				break;
			}

			}
			if (lock_cam)
				SDL_WarpMouseInWindow(window, SCR_W / 2, SCR_H / 2);

		}

		SDL_GL_SwapWindow(window); // swap buffers
		/*if (diff > 0) {
			SDL_Delay(diff);
		}*/

	}

	cleanup();
	engine->drop();

	// FIN LOOP PRINCIPAL
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;
}
