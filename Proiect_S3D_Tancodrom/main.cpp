#include <Windows.h>
#include <locale>
#include <codecvt>

#include <stdlib.h> // necesare pentru citirea shader-elor
#include <stdio.h>
#include <math.h>
#include <string>

#include <GL/glew.h>

#include <GLM.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <glfw3.h>
#include <gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include "Shader.h"
#include "Model.h"
#include "Camera.h"
#include <stb_image.h>

#pragma comment (lib, "glfw3dll.lib")
#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "OpenGL32.lib")
#pragma comment (lib, "assimp-vc143-mt.lib")

// settings
const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 800;

GLuint ProjMatrixLocation, ViewMatrixLocation, WorldMatrixLocation;
Camera* pCamera = nullptr;

unsigned int CreateTexture(const std::string& strTexturePath)
{
	unsigned int textureId = -1;

	// load image, create texture and generate mipmaps
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char* data = stbi_load(strTexturePath.c_str(), &width, &height, &nrChannels, 0);
	if (data) {
		GLenum format;
		if (nrChannels == 1)
			format = GL_RED;
		else if (nrChannels == 3)
			format = GL_RGB;
		else if (nrChannels == 4)
			format = GL_RGBA;

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else {
		std::cout << "Failed to load texture: " << strTexturePath << std::endl;
	}
	stbi_image_free(data);

	return textureId;
}
unsigned int planeVAO = 0;
void renderFloor()
{
	unsigned int planeVBO;

	if (planeVAO == 0) {
		// set up vertex data (and buffer(s)) and configure vertex attributes
		float planeVertices[] = {
			// positions            // normals         // texcoords
			500.0f, -0.0f,  500.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-500.0f, -0.0f,  500.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
			-500.0f, -0.0f, -500.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,

			500.0f, -0.0f,  500.0f,  0.0f, 1.0f, 0.0f,  25.0f,  0.0f,
			-500.0f, -0.0f, -500.0f,  0.0f, 1.0f, 0.0f,   0.0f, 25.0f,
			500.0f, -0.0f, -500.0f,  0.0f, 1.0f, 0.0f,  25.0f, 25.0f
		};
		// plane VAO
		glGenVertexArrays(1, &planeVAO);
		glGenBuffers(1, &planeVBO);
		glBindVertexArray(planeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
		glBindVertexArray(0);
	}

	glBindVertexArray(planeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}


void Cleanup()
{
	delete pCamera;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window, Model& piratObjModel);

// timing
double deltaTime = 0.0f;	// time between current frame and last frame
double lastFrame = 0.0f;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_A && action == GLFW_PRESS) {

	}
}

std::string ConvertWStringToString(const std::wstring& wstr) {
	int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	if (bufferSize == 0) {
		// Tratează eroarea de conversie
		// Poți arunca o excepție, returna un șir de caractere gol sau trata eroarea în alt mod
		return "";
	}

	std::string str(bufferSize, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &str[0], bufferSize, nullptr, nullptr);
	return str;
}

std::vector<Model> tankuri;

void PozitioneazaTankuri(Model &tank1, Model &tank2, Model &tank3, Model &tank4, Model &tank5, Model &tank6, Model &tank7, Model &tank8, Model &mountain1, Model& mountain2, Model& mountain3, Model& mountain4)
{
	tank1.SetPosition(glm::vec3(0.0f, 0.0f, 30.0f));
	tank1.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	tank1.SetRotationAngle(180.0f);
	tank2.SetPosition(glm::vec3(-12.0f, 0.0f, 20.0f));
	tank2.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	tank2.SetRotationAngle(180.0f);
	tank3.SetPosition(glm::vec3(-25.0f, 0.0f, 20.0f));
	tank3.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	tank3.SetRotationAngle(180.0f);
	tank4.SetPosition(glm::vec3(14.0f, 0.0f, 27.0f));
	tank4.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	tank4.SetRotationAngle(180.0f);

	tank5.SetPosition(glm::vec3(0.0f, 0.0f, -20.0f));
	tank5.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	//tank5.SetRotationAngle(180.0f); 
	tank6.SetPosition(glm::vec3(-12.0f, 0.0f, -25.0f));
	tank6.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	//tank6.SetRotationAngle(180.0f); 
	tank7.SetPosition(glm::vec3(-25.0f, 0.0f, -29.0f));
	tank7.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	//tank7.SetRotationAngle(180.0f);
	tank8.SetPosition(glm::vec3(14.0f, 0.0f, -30.0f));
	tank8.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	//tank8.SetRotationAngle(180.0f); 

	
	mountain1.SetPosition(glm::vec3(150.0f, -2.0f, 0.0f));
	mountain1.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	mountain1.SetScale(glm::vec3(25.0f));
	mountain2.SetPosition(glm::vec3(-150.0f, -2.0f, 0.0f));
	mountain2.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	mountain2.SetScale(glm::vec3(25.0f));
	mountain3.SetPosition(glm::vec3(0.0f, -2.0f, -150.0f));
	mountain3.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	mountain3.SetScale(glm::vec3(25.0f));
	mountain3.SetRotationAngle(90.0f);
	mountain4.SetPosition(glm::vec3(0.0f, -2.0f, 150.0f));
	mountain4.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	mountain4.SetScale(glm::vec3(25.0f));
	mountain4.SetRotationAngle(90.0f);
}

int main()
{
	// glfw: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// glfw window creation
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Incercare Model", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback); /// ultima mouse eroare
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, key_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glewInit();

	glEnable(GL_DEPTH_TEST);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	float vertices[] = {
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
		-0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
		-0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
		-0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
		-0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
		-0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
	};
	// first, configure the cube's VAO (and VBO)
	unsigned int VBO, cubeVAO;
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindVertexArray(cubeVAO);

	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// second, configure the light's VAO (VBO stays the same; the vertices are the same for the light object which is also a 3D cube)
	unsigned int lightVAO;
	glGenVertexArrays(1, &lightVAO);
	glBindVertexArray(lightVAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// note that we update the lamp's position attribute's stride to reflect the updated buffer data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // cursor haz
	// Create camera
	pCamera = new Camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0.0, 0.0, 3.0));

	glm::vec3 lightPos(0.0f, 2.0f, 1.0f);

	wchar_t buffer[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, buffer);

	std::wstring executablePath(buffer);
	std::wstring wscurrentPath = executablePath.substr(0, executablePath.find_last_of(L"\\/"));

	/*std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::string currentPath = converter.to_bytes(wscurrentPath);*/
	//std::wstring wscurrentPath; // Asume că aceasta este inițializată cu calea dorită
	std::string currentPath = ConvertWStringToString(wscurrentPath).c_str();			// de tinut minte asta

	const char* currentPathChr = currentPath.c_str();

	std::string path = (std::string(currentPathChr) + "\\Shaders\\PhongLight.vs").c_str();
	std::string path2 = (std::string(currentPathChr) + "\\Shaders\\PhongLight.vs").c_str();


	

	Shader lightingShader((currentPathChr + std::string("\\Shaders\\PhongLight.vs")).c_str(), ((currentPathChr + std::string("\\Shaders\\PhongLight.fs")).c_str()));
	Shader lampShader((currentPathChr + std::string("\\Shaders\\Lamp.vs")).c_str(), ((currentPathChr + std::string("\\Shaders\\Lamp.fs")).c_str()));
	//Shader lightingShader((currentPath + "\\Shaders\\ShadowMapping.vs").c_str(), (currentPath + "\\Shaders\\ShadowMapping.fs").c_str());
	//Shader lampShader((currentPath + "\\Shaders\\ShadowMappingDepth.vs").c_str(), (currentPath + "\\Shaders\\ShadowMappingDepth.fs").c_str());


	/*std::string objFileName = std::string(currentPathChr) + "\\Models\\CylinderProject.obj";
	Model objModel(objFileName, false);*/

	//std::string piratObjFileName = (currentPath + "\\Models\\Pirat\\Pirat.obj");
	//std::string piratObjFileName = (currentPath + "\\Models\\maimuta.obj");
	//std::string piratObjFileName = (currentPath + "\\Models\\14077_WWII_Tank_Germany_Panzer_III_v1_L2.obj");
	std::string piratObjFileName = (std::string(currentPathChr) + "\\Models\\Tiger.obj");
	//std::string mountainObjFileName = (std::string(currentPathChr) + "\\Models\\mountain\\mount.blend1.obj");
	std::string mountainObjFileName = (std::string(currentPathChr) + "\\Models\\mount.blend1.obj");
	//std::string piratObjFileName = (currentPath + "\\Models\\Human\\human.obj");
	//std::string piratObjFileName = (currentPath + "\\Models\\WWII_Tank_Germany_Panzer_III_v1_L2.123c56cb92d1-9485-44e9-a197-a7bddb48c29f\\14077_WWII_Tank_Germany_Panzer_III_v1_L2.obj");
	Model tank1(piratObjFileName, false);
	Model tank2(piratObjFileName, false);
	Model tank3(piratObjFileName, false);
	Model tank4(piratObjFileName, false);
	Model tank5(piratObjFileName, false);
	Model tank6(piratObjFileName, false);
	Model tank7(piratObjFileName, false);
	Model tank8(piratObjFileName, false);
	
	Model mountain1(mountainObjFileName, false);
	Model mountain2(mountainObjFileName, false);
	Model mountain3(mountainObjFileName, false);
	Model mountain4(mountainObjFileName, false);


	unsigned int floorTexture = CreateTexture(std::string(currentPathChr) + "\\ColoredFloor.png");
	


	float radius = 80.0f; // Raza cercului pe care se va rota lumina
	float speed = 0.05f;

	//tankuri.push_back(tank1);
	tankuri.push_back(tank2);
	tankuri.push_back(tank3);
	tankuri.push_back(tank4);


	PozitioneazaTankuri(tank1, tank2, tank3, tank4, tank5, tank6, tank7, tank8, mountain1, mountain2, mountain3, mountain4);

	//float movementSpeed = 0.1f; // ajustează la viteza dorită
	//glm::vec3 movementDirection(1.0f, 0.0f, 0.0f); // mutare pe axa X

	//renderFloor();
	// Render loop
	while (!glfwWindowShouldClose(window)) {
		// Per-frame time logic
		double currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;


		// Input
		//processInput(window);
		processInput(window, tank1);

		// Clear buffers
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Update light position
		float time = glfwGetTime();
		float lightX = radius * cos(speed * time);
		float lightZ = radius * sin(speed * time);
		lightPos.x = lightX;
		lightPos.y = fabs(lightZ);

		//renderFloor();
		// Use lighting shader
		lightingShader.Use();

		if (lightZ >= 0) {
			lightingShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);
			lightingShader.SetVec3("objectColor", 0.5f, 1.0f, 0.3f);
		}
		else {
			lightingShader.SetVec3("lightColor", 0.8f, 0.8f, 0.8f);
			lightingShader.SetVec3("objectColor", 0.8f, 0.8f, 0.3f);
			lightPos.x = -lightPos.x;
		}

		lightingShader.SetVec3("lightPos", lightPos);
		//glBindTexture(GL_TEXTURE_2D, floorTexture);
		lightingShader.SetVec3("viewPos", pCamera->GetPosition());
		
		lightingShader.SetMat4("projection", pCamera->GetProjectionMatrix());
		lightingShader.SetMat4("view", pCamera->GetViewMatrix());
		
		////Calculate new model position
		/*glm::vec3 newPosition = piratObjModel.GetPosition() + (movementDirection * movementSpeed * static_cast<float>(deltaTime));
		piratObjModel.SetPosition(newPosition);*/
		
		// Set model matrix and draw the model
		lightingShader.SetMat4("model", mountain1.GetModelMatrix());
		mountain1.Draw(lightingShader);
		lightingShader.SetMat4("model", mountain2.GetModelMatrix());
		mountain2.Draw(lightingShader);
		lightingShader.SetMat4("model", mountain3.GetModelMatrix());
		mountain3.Draw(lightingShader);
		lightingShader.SetMat4("model", mountain4.GetModelMatrix());
		mountain4.Draw(lightingShader);
		lightingShader.SetMat4("model", tank1.GetModelMatrix());
		tank1.Draw(lightingShader);
		lightingShader.SetMat4("model", tank2.GetModelMatrix());
		tank2.Draw(lightingShader);
		lightingShader.SetMat4("model", tank3.GetModelMatrix());
		tank3.Draw(lightingShader);
		lightingShader.SetMat4("model", tank4.GetModelMatrix());
		tank4.Draw(lightingShader);
		lightingShader.SetMat4("model", tank5.GetModelMatrix());
		tank5.Draw(lightingShader);
		lightingShader.SetMat4("model", tank6.GetModelMatrix());
		tank6.Draw(lightingShader);
		lightingShader.SetMat4("model", tank7.GetModelMatrix());
		tank7.Draw(lightingShader);
		lightingShader.SetMat4("model", tank8.GetModelMatrix());
		tank8.Draw(lightingShader);
		
		renderFloor();
		
		
		// Use lamp shader
		lampShader.Use();
		lampShader.SetMat4("projection", pCamera->GetProjectionMatrix());
		lampShader.SetMat4("view", pCamera->GetViewMatrix());

		//glBindTexture(GL_TEXTURE_2D, floorTexture);

		// Set light model matrix and draw the lamp object
		glm::mat4 lightModel = glm::translate(glm::mat4(1.0), lightPos);
		lightModel = glm::scale(lightModel, glm::vec3(0.5f)); // a smaller cube
		lampShader.SetMat4("model", lightModel);
		glBindTexture(GL_TEXTURE_2D, floorTexture);
		glActiveTexture(GL_TEXTURE1);
		glBindVertexArray(lightVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// Swap buffers and poll IO events
		glfwSwapBuffers(window);
		glfwPollEvents();

	}


	Cleanup();

	glDeleteVertexArrays(1, &cubeVAO);
	glDeleteVertexArrays(1, &lightVAO);
	glDeleteBuffers(1, &VBO);

	// glfw: terminate, clearing all previously allocated GLFW resources
	glfwTerminate();
	return 0;
}

glm::vec3 rotateVector(const glm::vec3& vec, float angle, const glm::vec3& axis) {
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, axis);
	return glm::vec3(rotationMatrix * glm::vec4(vec, 1.0f));
}

bool tankIsSelected = false;
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window, Model& piratObjModel)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (tankIsSelected) {
		glm::vec3 movementDirection(0.0f);
		float rotationAngle = piratObjModel.GetRotationAngle();

		// Calculul direcției de mișcare în funcție de unghiul de rotație
		float radianRotationAngle = glm::radians(-rotationAngle); // Convertim unghiul în radiani
		float cosAngle = cos(radianRotationAngle);
		float sinAngle = sin(radianRotationAngle);

		// Rotăm direcția îndreptată înainte în jurul axei y
		glm::vec3 forwardDirection(
			0.0f,
			0.0f,
			1.0f
		);
		glm::vec3 rotatedForwardDirection(
			forwardDirection.x * cosAngle + forwardDirection.z * sinAngle,
			forwardDirection.y,
			forwardDirection.x * sinAngle - forwardDirection.z * cosAngle
		);

		// Updatează direcția de mișcare bazată pe tastele apăsate
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			movementDirection -= rotatedForwardDirection; // Înainte
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			movementDirection += rotatedForwardDirection; // Înapoi

		// Aplică viteza de mișcare
		float movementSpeed = 0.5f; // Ajustează după necesități
		glm::vec3 newPosition = piratObjModel.GetPosition() + (movementDirection * movementSpeed * static_cast<float>(deltaTime));
		piratObjModel.SetPosition(newPosition);

		// Inițializează viteza și unghiul de rotație
		float rotationSpeed = 20.0f; // Ajustează după necesități
		glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f); // Axul de rotație implicit (în jurul axei Y)

		// Rotație la stânga (tasta A)
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		{
			rotationAngle += rotationSpeed * static_cast<float>(deltaTime); // Adaugă unghiul de rotație
			rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Setează axa de rotație la axa Y
		}
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		{
			rotationAngle -= rotationSpeed * static_cast<float>(deltaTime); // Scade unghiul de rotație
			rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Setează axa de rotație la axa Y
		}

		// Setează unghiul și axa de rotație
		piratObjModel.SetRotationAngle(rotationAngle);
		piratObjModel.SetRotationAxis(rotationAxis);
	}

	//---- modificare tankIsSelected ---- 
	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
	{
		tankIsSelected = true;
	
	}
	if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
	{
		tankIsSelected = false;
		
	}


	//----------------------  CAMERA  ------------------------------------
	if (tankIsSelected)
	{
		// Obținem poziția și rotația tankului
		glm::vec3 tankPosition = piratObjModel.GetPosition();
		float tankRotationAngle = piratObjModel.GetRotationAngle();
		glm::vec3 tankRotationAxis = piratObjModel.GetRotationAxis();
		// Offset-ul camerei față de tank
		glm::vec3 cameraOffset = glm::vec3(0.0f, 8.0f, -20.0f); // Offset-ul inițial
		// Rotăm offset-ul camerei în funcție de rotația tankului
		cameraOffset = rotateVector(cameraOffset, glm::radians(tankRotationAngle), tankRotationAxis);
		//pCamera->SetYaw(tankRotationAngle);
		std::cout << tankRotationAngle <<"\n";
		// Poziția camerei va fi poziția tankului plus offset-ul
		glm::vec3 cameraPosition = tankPosition + cameraOffset;
		// Setăm poziția camerei
		pCamera->SetPosition(cameraPosition);
		// Orientăm camera spre tank
		pCamera->LookAt(tankPosition);
	}

	if (!tankIsSelected)
	{
		if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
			pCamera->ProcessKeyboard(Camera::FORWARD, (float)deltaTime * 5);
		if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
			pCamera->ProcessKeyboard(Camera::BACKWARD, (float)deltaTime * 5);
		if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
			pCamera->ProcessKeyboard(Camera::LEFT, (float)deltaTime * 5);
		if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
			pCamera->ProcessKeyboard(Camera::RIGHT, (float)deltaTime * 5);
		if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
			pCamera->ProcessKeyboard(Camera::UP, (float)deltaTime * 5);
		if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
			pCamera->ProcessKeyboard(Camera::DOWN, (float)deltaTime * 5);

		if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			int width, height;
			glfwGetWindowSize(window, &width, &height);
			pCamera->Reset(width, height);
		}
	}
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	pCamera->Reshape(width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if(!tankIsSelected)
		pCamera->MouseControl((float)xpos, (float)ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yOffset)
{
	pCamera->ProcessMouseScroll((float)yOffset);
}
