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
#include "Helicopter.h"
#include "Tank.h"

#pragma comment (lib, "glfw3dll.lib")
#pragma comment (lib, "glew32.lib")
#pragma comment (lib, "OpenGL32.lib")
#pragma comment (lib, "assimp-vc143-mt.lib")


// Define a simple Color struct
struct Color {
	float r, g, b; // Red, green, and blue components

	// Constructor
	Color(float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
};

// Linear interpolation function for colors
Color lerp(const Color& x, const Color& y, float t) {
	float r = x.r + (y.r - x.r) * t;
	float g = x.g + (y.g - x.g) * t;
	float b = x.b + (y.b - x.b) * t;
	return Color(r, g, b);
}

// Function to calculate the interpolation factor based on time of day
float calculateInterpolationFactor(float timeOfDay) {
	// Calculate the interpolation factor based on time of day (0 to 1)
	// Ensure that the interpolation factor resets to 0 at the start of each day
	const float hoursPerDay = 48.0f;
	return fmod(timeOfDay, hoursPerDay) / hoursPerDay;
}
void updateBackgroundColor(float timeOfDay) {
	// Define key colors
	Color lightBlue(0.5f, 0.7f, 1.0f);    // Light blue
	Color purple(0.988f, 0.831f, 0.967f);       // Purple
	Color darkBlue(0.0f, 0.0f, 0.2f);     // Dark blue
	Color sunrise(0.78f, 0.73f, 0.80f);       // Orange

	// Calculate interpolation factor
	float interpolationFactor = calculateInterpolationFactor(timeOfDay);

	// Interpolate between colors
	Color interpolatedColor(0.0f, 0.0f, 0.0f);
	if (interpolationFactor < 0.1f) {
		interpolatedColor = lerp(darkBlue, sunrise, interpolationFactor  * 10.0f);
	}
	else if (interpolationFactor < 0.2f) {
		interpolatedColor = lerp(sunrise, lightBlue, (interpolationFactor - 0.1f) * 10.0f);
	}
	else if (interpolationFactor < 0.4f) {
		interpolatedColor = lerp(lightBlue, lightBlue, (interpolationFactor - 0.3f) * 4.0f);
	}
	else if (interpolationFactor < 0.5f) {
		interpolatedColor = lerp(lightBlue, darkBlue, (interpolationFactor - 0.4f) * 10.0f);
	}
	else {
		interpolatedColor = lerp(darkBlue, darkBlue, (interpolationFactor - 0.5f) * 4.0f);
	}

	// Set the background color using glClearColor
	glClearColor(interpolatedColor.r, interpolatedColor.g, interpolatedColor.b, 1.0f);
}

float calculateTimeOfDay(glm::vec3 lightPos) {
	// Define reference positions for sunrise and sunset
	glm::vec3 sunrisePos(0.0f, 0.0f, 0.0f); // Example: Sun rises from the center
	glm::vec3 sunsetPos(0.0f, 0.0f, -1.0f); // Example: Sun sets along the negative z-axis

	// Calculate the angle between the light position and the sunrise position
	glm::vec3 lightDir = glm::normalize(lightPos);
	glm::vec3 sunriseDir = glm::normalize(sunrisePos);
	float dotProduct = glm::dot(lightDir, sunriseDir);
	float angle = glm::acos(dotProduct);

	// Map the angle to the time of day (0 to 1)
	// Assuming the sun moves from sunrise (0.0) to sunset (0.5) and back to sunrise (1.0)
	float timeOfDay = angle / glm::pi<float>();

	// Adjust timeOfDay if the sun is setting (after 180 degrees)
	if (lightPos.z < 0.0f) {
		timeOfDay = 1.0f - timeOfDay;
	}

	return timeOfDay;
}

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
			500.0f, -0.0f,  500.0f,  0.0f, 1.0f, 0.0f,  75.0f,  0.0f,
			-500.0f, -0.0f,  500.0f,  0.0f, 1.0f, 0.0f,   0.0f,  0.0f,
			-500.0f, -0.0f, -500.0f,  0.0f, 1.0f, 0.0f,   0.0f, 75.0f,

			500.0f, -0.0f,  500.0f,  0.0f, 1.0f, 0.0f,  75.0f,  0.0f,
			-500.0f, -0.0f, -500.0f,  0.0f, 1.0f, 0.0f,   0.0f, 75.0f,
			500.0f, -0.0f, -500.0f,  0.0f, 1.0f, 0.0f,  75.0f, 75.0f
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
void processInput(GLFWwindow* window, std::vector<Tank>& tanks);

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

void RenderModels(Shader& lightingShader, std::vector<Tank>& tanks,
	std::vector<Model>& mountains,
	Helicopter& helicopter1, Helicopter& helicopter2,
	std::vector<Model>& clouds
)
{
	glBindTexture(GL_TEXTURE_2D, 2);
	for (int i = 0; i < 4; i++) 
	{
		lightingShader.SetMat4("model", mountains[i].GetModelMatrix());
		mountains[i].Draw(lightingShader);
	}

	glBindTexture(GL_TEXTURE_2D, 1);
	for (int i = 0; i < tanks.size(); i++) {
		lightingShader.SetMat4("model", tanks[i].Body.GetModelMatrix());
		tanks[i].Body.Draw(lightingShader);
		lightingShader.SetMat4("model", tanks[i].Head.GetModelMatrix());
		tanks[i].Head.Draw(lightingShader);
	}
	/*lightingShader.SetMat4("model", tank1.Body.GetModelMatrix());
	tanks[i].Body.Draw(lightingShader);
	lightingShader.SetMat4("model", tank1.Head.GetModelMatrix());
	tank1.Head.Draw(lightingShader);
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
	tank8.Draw(lightingShader);*/
	glBindTexture(GL_TEXTURE_2D, 2);
	renderFloor();
	glBindTexture(GL_TEXTURE_2D, 1);


	lightingShader.SetMat4("model", helicopter1.Body.GetModelMatrix());
	helicopter1.Body.Draw(lightingShader);
	lightingShader.SetMat4("model", helicopter1.ProppelerUp.GetModelMatrix());
	helicopter1.ProppelerUp.Draw(lightingShader);
	lightingShader.SetMat4("model", helicopter1.ProppelerBack.GetModelMatrix());
	helicopter1.ProppelerBack.Draw(lightingShader);
	lightingShader.SetMat4("model", helicopter2.Body.GetModelMatrix());
	helicopter2.Body.Draw(lightingShader);
	lightingShader.SetMat4("model", helicopter2.ProppelerUp.GetModelMatrix());
	helicopter2.ProppelerUp.Draw(lightingShader);
	lightingShader.SetMat4("model", helicopter2.ProppelerBack.GetModelMatrix());
	helicopter2.ProppelerBack.Draw(lightingShader);


	glBindTexture(GL_TEXTURE_2D, 4);
	for (int i = 0; i < 25; i++)
	{
		lightingShader.SetMat4("model", clouds[i].GetModelMatrix());
		clouds[i].Draw(lightingShader);
	}
}

void PozitionateModels(std::vector<Tank>& tanks,
	std::vector<Model>& mountains,
	Helicopter& helicopter1, Helicopter& helicopter2,
	std::vector<Model>& clouds
)
{
	//---- Tancuri ----
	tanks[0].Body.SetPosition(glm::vec3(0.0f, 0.0f, 30.0f));
	tanks[0].Head.SetPosition(glm::vec3(0.0f, 0.0f, 30.0f));
	tanks[1].Body.SetPosition(glm::vec3(-12.0f, 0.0f, 20.0f));
	tanks[1].Head.SetPosition(glm::vec3(-12.0f, 0.0f, 20.0f));
	tanks[2].Body.SetPosition(glm::vec3(-25.0f, 0.0f, 20.0f));
	tanks[2].Head.SetPosition(glm::vec3(-25.0f, 0.0f, 20.0f));
	tanks[3].Body.SetPosition(glm::vec3(14.0f, 0.0f, 27.0f));
	tanks[3].Head.SetPosition(glm::vec3(14.0f, 0.0f, 27.0f));
	tanks[4].Body.SetPosition(glm::vec3(0.0f, 0.0f, -20.0f));
	tanks[4].Head.SetPosition(glm::vec3(0.0f, 0.0f, -20.0f));

	tanks[5].Body.SetPosition(glm::vec3(-12.0f, 0.0f, -25.0f));
	tanks[5].Head.SetPosition(glm::vec3(-12.0f, 0.0f, -25.0f));
	tanks[6].Body.SetPosition(glm::vec3(-25.0f, 0.0f, -29.0f));
	tanks[6].Head.SetPosition(glm::vec3(-25.0f, 0.0f, -29.0f));
	tanks[7].Body.SetPosition(glm::vec3(14.0f, 0.0f, -30.0f));
	tanks[7].Head.SetPosition(glm::vec3(14.0f, 0.0f, -30.0f));

	for (int i = 0; i < tanks.size(); i++)
	{
		tanks[i].Body.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
		tanks[i].Head.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
		if (i < 4) {
			tanks[i].Body.SetRotationAngle(180.0f);
			tanks[i].Head.SetRotationAngle(180.0f);
		}
		else {
			tanks[i].Body.SetRotationAngle(0.0f);
			tanks[i].Head.SetRotationAngle(0.0f);
		}
	}

	//---- Munti ----
	mountains[0].SetPosition(glm::vec3(150.0f, -2.0f, 0.0f));
	mountains[0].SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	mountains[1].SetPosition(glm::vec3(-150.0f, -2.0f, 0.0f));
	mountains[1].SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	mountains[2].SetPosition(glm::vec3(0.0f, -2.0f, -150.0f));
	mountains[2].SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	mountains[3].SetPosition(glm::vec3(0.0f, -2.0f, 150.0f));
	mountains[3].SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	for (int i = 0; i < 4; i++) {
		mountains[i].SetScale(glm::vec3(25.0f));
		mountains[i].SetRotationAngle(0.0f);
	}

	//---- Elicoptere ----
	helicopter1.Body.SetPosition(glm::vec3(20.25f, 20.0f, -30.0f));
	helicopter1.Body.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	helicopter1.Body.SetRotationAngle(0.0f);
	helicopter1.ProppelerUp.SetPosition(glm::vec3(20.0f, 20.0f, -30.0f));
	helicopter1.ProppelerUp.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	helicopter1.ProppelerBack.SetPosition(glm::vec3(20.5f, 23.63f, -41.27f));
	helicopter1.ProppelerBack.SetRotationAxis(glm::vec3(1.0f, 0.0f, 0.0f));

	helicopter2.Body.SetPosition(glm::vec3(19.75f, 20.0f, 30.0f));
	helicopter2.Body.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	helicopter2.Body.SetRotationAngle(180.0f);
	helicopter2.ProppelerUp.SetPosition(glm::vec3(20.0f, 20.0f, 30.0f));
	helicopter2.ProppelerUp.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	helicopter2.ProppelerBack.SetPosition(glm::vec3(19.5f, 23.63f, 41.27f));
	helicopter2.ProppelerBack.SetRotationAxis(glm::vec3(1.0f, 0.0f, 0.0f));

	//---- Nori ----
	clouds[0].SetPosition(glm::vec3(30.0f, 140.0f, 210.0f));
	clouds[0].SetScale(glm::vec3(0.35f));
	clouds[1].SetPosition(glm::vec3(-70.0f, 140.0f, 50.0f));
	clouds[1].SetScale(glm::vec3(0.2f));
	clouds[2].SetPosition(glm::vec3(85.0f, 140.0f, -35.0f));
	clouds[2].SetScale(glm::vec3(0.55f));
	clouds[3].SetPosition(glm::vec3(20.0f, 150.0f, 205.0f));
	clouds[3].SetScale(glm::vec3(0.35f));
	clouds[4].SetPosition(glm::vec3(95.0f, 150.0f, -100.0f));
	clouds[4].SetScale(glm::vec3(0.45f));
	clouds[5].SetPosition(glm::vec3(-50.0f, 140.0f, -90.0f));
	clouds[5].SetScale(glm::vec3(0.35f));
	clouds[6].SetPosition(glm::vec3(120.0f, 160.0f, 65.0f));
	clouds[6].SetScale(glm::vec3(0.25f));
	clouds[7].SetPosition(glm::vec3(270.0f, 150.0f, -130.0f));
	clouds[7].SetScale(glm::vec3(0.65f));
	clouds[8].SetPosition(glm::vec3(285.0f, 140.0f, -40.0f));
	clouds[8].SetScale(glm::vec3(0.25f));
	clouds[9].SetPosition(glm::vec3(85.0f, 150.0f, 135.0f));
	clouds[9].SetScale(glm::vec3(0.45f));
	clouds[10].SetPosition(glm::vec3(-220.0f, 160.0f, -125.0f));
	clouds[10].SetScale(glm::vec3(0.25f));
	clouds[11].SetPosition(glm::vec3(295.0f, 150.0f, 100.0f));
	clouds[11].SetScale(glm::vec3(0.25f));
	clouds[12].SetPosition(glm::vec3(250.0f, 150.0f, 90.0f));
	clouds[12].SetRotationAngle(180.0f);
	clouds[12].SetScale(glm::vec3(0.25f));
	clouds[13].SetPosition(glm::vec3(-120.0f, 140.0f, 65.0f));
	clouds[13].SetScale(glm::vec3(0.55f));
	clouds[14].SetPosition(glm::vec3(-270.0f, 140.0f, 10.0f));
	clouds[14].SetScale(glm::vec3(0.45f));
	clouds[15].SetPosition(glm::vec3(-130.0f, 160.0f, 250.0f));
	clouds[15].SetScale(glm::vec3(0.40f));
	clouds[16].SetPosition(glm::vec3(150.0f, 160.0f, -250.0f));
	clouds[16].SetScale(glm::vec3(0.35f));
	clouds[17].SetPosition(glm::vec3(-20.0f, 150.0f, 10.0f));
	clouds[17].SetScale(glm::vec3(0.45f));
	clouds[18].SetPosition(glm::vec3(-50.0f, 150.0f, -230.0f));
	clouds[18].SetScale(glm::vec3(0.35f));
	clouds[19].SetPosition(glm::vec3(-250.0f, 150.0f, -130.0f));
	clouds[19].SetScale(glm::vec3(0.85f));
	clouds[20].SetPosition(glm::vec3(250.0f, 150.0f, -130.0f));
	clouds[20].SetScale(glm::vec3(0.85f));
	clouds[21].SetPosition(glm::vec3(250.0f, 150.0f, 160.0f));
	clouds[21].SetScale(glm::vec3(0.85f));
	clouds[22].SetPosition(glm::vec3(-150.0f, 150.0f, 130.0f));
	clouds[22].SetScale(glm::vec3(0.85f));
	clouds[23].SetPosition(glm::vec3(-250.0f, 150.0f, 130.0f));
	clouds[23].SetScale(glm::vec3(0.85f));
	clouds[24].SetPosition(glm::vec3(250.0f, 150.0f, -160.0f));
	clouds[24].SetScale(glm::vec3(0.85f));
	for (int i = 0; i < 25; i++) {
		clouds[i].SetRotationAngle(180.0f);
		clouds[i].SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	}
}

void moveClouds(std::vector<Model>& clouds)
{
	if (clouds[14].GetPosition().x > -400.0f && clouds[16].GetPosition().x > -400.0f)
		for (int i = 0; i < 25; i++)
			clouds[i].SetPosition(clouds[i].GetPosition() + glm::vec3(-0.002f, 0.0f, -0.002f));

}


void rotate_elice(Model& helicpter3_elice, double deltaTime)
{
	helicpter3_elice.SetRotationAngle(helicpter3_elice.GetRotationAngle() + 100.0f * deltaTime * 250);
}
void rotate_elice_spate(Model& helicpter3_elice_spate, double deltaTime)
{
	helicpter3_elice_spate.SetRotationAngle(helicpter3_elice_spate.GetRotationAngle() + 100.0f * deltaTime * 50);
}
void rotateElice(Helicopter& helicopter1, Helicopter& helicopter2) {
	rotate_elice(helicopter1.ProppelerUp, deltaTime);
	rotate_elice(helicopter2.ProppelerUp, deltaTime);

	rotate_elice_spate(helicopter1.ProppelerBack, deltaTime);
	rotate_elice_spate(helicopter2.ProppelerBack, deltaTime);
}
bool isNight = false;

int main()
{
	// key colors for sky gradient
	Color lightBlue(0.5f, 0.7f, 1.0f);    // Light blue
	Color purple(0.5f, 0.0f, 0.5f);       // Purple
	Color darkBlue(0.0f, 0.0f, 0.2f);     // Dark blue
	Color orange(1.0f, 0.5f, 0.0f);       // Orange

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
	pCamera = new Camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0.0, 2.0, 3.0));

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

	//Shader lightingShader((currentPathChr + std::string("\\Shaders\\PhongLight.vs")).c_str(), ((currentPathChr + std::string("\\Shaders\\PhongLight.fs")).c_str()));
	Shader lampShader((currentPathChr + std::string("\\Shaders\\Lamp.vs")).c_str(), ((currentPathChr + std::string("\\Shaders\\Lamp.fs")).c_str()));
	Shader lightingShader((currentPath + "\\Shaders\\ShadowMapping.vs").c_str(), (currentPath + "\\Shaders\\ShadowMapping.fs").c_str());
	//Shader lampShader((currentPath + "\\Shaders\\ShadowMappingDepth.vs").c_str(), (currentPath + "\\Shaders\\ShadowMappingDepth.fs").c_str());

	//std::string piratObjFileName = (currentPath + "\\Models\\maimuta.obj");
	//std::string piratObjFileName = (currentPath + "\\Models\\14077_WWII_Tank_Germany_Panzer_III_v1_L2.obj");
	std::string piratObjFileName = (std::string(currentPathChr) + "\\Models\\Tiger.obj");
	std::string tank_bodyObjFileName = (std::string(currentPathChr) + "\\Models\\Tiger_body.obj");
	std::string tank_turretObjFileName = (std::string(currentPathChr) + "\\Models\\Tiger_turret.obj");

	//std::string mountainObjFileName = (std::string(currentPathChr) + "\\Models\\mountain\\mount.blend1.obj");
	std::string mountainObjFileName = (std::string(currentPathChr) + "\\Models\\mount.blend1.obj");
	//std::string piratObjFileName = (currentPath + "\\Models\\Human\\human.obj");
	//std::string piratObjFileName = (currentPath + "\\Models\\WWII_Tank_Germany_Panzer_III_v1_L2.123c56cb92d1-9485-44e9-a197-a7bddb48c29f\\14077_WWII_Tank_Germany_Panzer_III_v1_L2.obj");
	//std::string helicopterObjFileName = (std::string(currentPathChr) + "\\Models\\Heli_bell_nou.obj");
	std::string helicopterObjFileName = (std::string(currentPathChr) + "\\Models\\Heli_Bell\\Heli_bell_nou.obj");
	std::string heli_bodyObjFileName = (std::string(currentPathChr) + "\\Models\\Helicopter_body2.obj");
	std::string heli_eliceObjFileName = (std::string(currentPathChr) + "\\Models\\Helicopter_elice2.obj");
	std::string heli_elice_spateObjFileName = (std::string(currentPathChr) + "\\Models\\Helicopter_elice_back.obj");

	std::string cloudObjFileName = (std::string(currentPathChr) + "\\Models\\cumulus00.obj");
	std::string cloud2ObjFileName = (std::string(currentPathChr) + "\\Models\\cumulus01.obj");
	std::string cloud3ObjFileName = (std::string(currentPathChr) + "\\Models\\cumulus02.obj");
	std::string cloud4ObjFileName = (std::string(currentPathChr) + "\\Models\\altostratus00.obj");
	std::string cloud5ObjFileName = (std::string(currentPathChr) + "\\Models\\altostratus01.obj");


	//---- Creare Modele
	/*Tank tank1(Model(tank_bodyObjFileName, false), Model(tank_turretObjFileName, false));
	Tank tank2(Model(tank_bodyObjFileName, false), Model(tank_turretObjFileName, false));
	Tank tank3(Model(tank_bodyObjFileName, false), Model(tank_turretObjFileName, false));
	Tank tank4(Model(tank_bodyObjFileName, false), Model(tank_turretObjFileName, false));
	Tank tank5(Model(tank_bodyObjFileName, false), Model(tank_turretObjFileName, false));
	Tank tank6(Model(tank_bodyObjFileName, false), Model(tank_turretObjFileName, false));
	Tank tank7(Model(tank_bodyObjFileName, false), Model(tank_turretObjFileName, false));
	Tank tank8(Model(tank_bodyObjFileName, false), Model(tank_turretObjFileName, false));*/
	/*Model tank2(piratObjFileName, false);
	Model tank3(piratObjFileName, false);
	Model tank4(piratObjFileName, false);
	Model tank5(piratObjFileName, false);
	Model tank6(piratObjFileName, false);
	Model tank7(piratObjFileName, false);
	Model tank8(piratObjFileName, false);*/
	std::vector<Tank> tanks;
	tanks.insert(tanks.end(), 8, Tank(Model(tank_bodyObjFileName, false), Model(tank_turretObjFileName, false)));

	Helicopter helicopter1(Model(heli_bodyObjFileName, false), Model(heli_eliceObjFileName, false) , Model(heli_elice_spateObjFileName, false));
	Helicopter helicopter2(Model(heli_bodyObjFileName, false), Model(heli_eliceObjFileName, false), Model(heli_elice_spateObjFileName, false));

	std::vector<Model> clouds;
	clouds.insert(clouds.end(), 7, Model(cloudObjFileName, false));
	clouds.insert(clouds.end(), 4, Model(cloud2ObjFileName, false));
	clouds.insert(clouds.end(), 8, Model(cloud3ObjFileName, false));
	clouds.insert(clouds.end(), 1, Model(cloud4ObjFileName, false));
	clouds.insert(clouds.end(), 5, Model(cloud5ObjFileName, false));
	std::vector<Model> mountains(4, Model(mountainObjFileName, false));

	unsigned int floorTexture = CreateTexture(std::string(currentPathChr) + "\\ColoredFloor.png");
	unsigned int mountainTexture = CreateTexture(std::string(currentPathChr) + "\\Models\\mountain\\ground_grass_3264_4062_Small.jpg");
	unsigned int cloudTexture = CreateTexture(std::string(currentPathChr) + "\\Models\\clouds\\white.jpg");

	float radius = 350.0f; // Raza cercului pe care se va rota lumina
	float speed = 0.132f;



	PozitionateModels(
		tanks,
		mountains,
		helicopter1, helicopter2,
		clouds
	);

	// Render loop
	while (!glfwWindowShouldClose(window)) {
		// Per-frame time logic
		double currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		moveClouds(clouds);
		rotateElice(helicopter1, helicopter2);
		
		// Input
		//processInput(window);
				// aici ar cam trb modificat pentru a putea controla toate tancurile, acum se poate controla doar tancul tank1

		// Clear buffers
		float timeOfDay = glfwGetTime(); // Adjust this based on your time scale
		updateBackgroundColor(timeOfDay);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		

		// Update light position
		float time = glfwGetTime();
		float lightX = radius * cos(speed * time);
		float lightZ = radius * sin(speed * time);
		lightPos.x = lightX;
		lightPos.y = fabs(lightZ);

		// Use lighting shader
		lightingShader.Use();

		if (lightX == -350) {
			isNight = !isNight;
		}

		if (lightZ < 0) {
			lightPos.x = -lightPos.x;
			
		}

		// change between day and night

		if (isNight)
		{
			lightingShader.SetVec3("lightColor", 0.7f, 0.7f, 1.0f);

			lightingShader.SetFloat("Ka", 0.1);
			lightingShader.SetFloat("Kd", 0.3);
			lightingShader.SetFloat("Ks", 0.3);
		}
		else
		{
			lightingShader.SetVec3("lightColor", 1.0f, 1.0f, 0.9f);

			lightingShader.SetFloat("Ka", 0.2f);
			lightingShader.SetFloat("Kd", 0.7f);
			lightingShader.SetFloat("Ks", 0.5f);
		}

		lightingShader.SetVec3("lightPos", lightPos);
		lightingShader.SetVec3("viewPos", pCamera->GetPosition());

		lightingShader.SetMat4("projection", pCamera->GetProjectionMatrix());
		lightingShader.SetMat4("view", pCamera->GetViewMatrix());


		// Set model matrix and draw the model
		RenderModels(
			lightingShader,
			tanks,
			mountains,
			helicopter1, helicopter2,
			clouds
		);
		processInput(window, tanks);

		// Use lamp shader
		lampShader.Use();
		lampShader.SetMat4("projection", pCamera->GetProjectionMatrix());
		lampShader.SetMat4("view", pCamera->GetViewMatrix());

		// Set light model matrix and draw the lamp object
		glm::mat4 lightModel = glm::translate(glm::mat4(1.0), lightPos);
		if(isNight)
			lightModel = glm::scale(lightModel, glm::vec3(7.0f)); // a smaller cube
		else
			lightModel = glm::scale(lightModel, glm::vec3(10.0f)); // a smaller cube
		//lightModel = glm::scale(lightModel, glm::vec3(0.5f)); // a smaller cube
		lampShader.SetMat4("model", lightModel);

		//glBindTexture(GL_TEXTURE_2D, floorTexture);
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

bool IsAnObjectSelected(std::vector<Tank>& tanks) {
	for (int i = 0; i < tanks.size(); i++) {
		if (tanks[i].GetIsSelected())
			return true;
	}
	return false;
}
void SetObjSelectedOrNot(std::vector<Tank>& tanks, int tankNumber) {
	tankNumber -= 1;
	/*for (int i = 0; i < tanks.size(); i++) {
		if (i == tankNumber) {
			tanks[i].SetIsSelected(!tanks[i].GetIsSelected());
		}
		else if(tanks[i].GetIsSelected()) {
			tanks[i].SetIsSelected(false);
		}
	}*/
	
	for (int i = 0; i < tanks.size(); i++) {
		if (i != tankNumber)
			tanks[i].SetIsSelected(false);
		else
			tanks[tankNumber].SetIsSelected(!tanks[tankNumber].GetIsSelected());
	}
	
}

/*

2

3

*/
glm::vec3 rotateVector(const glm::vec3& vec, float angle, const glm::vec3& axis) {
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, axis);
	return glm::vec3(rotationMatrix * glm::vec4(vec, 1.0f));
}

bool tankIsSelected = false;
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window, std::vector<Tank>& tanks)
{
	//---- EXIT APP ----
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		SetObjSelectedOrNot(tanks, 1);
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		SetObjSelectedOrNot(tanks, 2);
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		SetObjSelectedOrNot(tanks, 3);
	if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		SetObjSelectedOrNot(tanks, 4);
	if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
		SetObjSelectedOrNot(tanks, 5);
	if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
		SetObjSelectedOrNot(tanks, 6);
	if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
		SetObjSelectedOrNot(tanks, 7);
	if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS)
		SetObjSelectedOrNot(tanks, 8);


	//---- Control miscari model(tank)
	tankIsSelected = IsAnObjectSelected(tanks);
	if(IsAnObjectSelected(tanks))
	for (int i = 0; i < tanks.size(); i++) 
	{
		if (tanks[i].GetIsSelected()) 
		{
			glm::vec3 movementDirectionBody(0.0f);
			glm::vec3 movementDirectionHead(0.0f);
			float rotationAngleBody = tanks[i].Body.GetRotationAngle();
			float rotationAngleHead = tanks[i].Head.GetRotationAngle();

			// Calculul direcției de mișcare în funcție de unghiul de rotație
			float radianRotationAngleBody = glm::radians(-rotationAngleBody); // Convertim unghiul în radiani
			float cosAngleBody = cos(radianRotationAngleBody);
			float sinAngleBody = sin(radianRotationAngleBody);

			float radianRotationAngleHead = glm::radians(-rotationAngleHead); // Convertim unghiul în radiani
			float cosAngleHead = cos(radianRotationAngleHead);
			float sinAngleHead = sin(radianRotationAngleHead);

			glm::vec3 forwardDirection(0.0f, 0.0f, 1.0f);
			glm::vec3 rotatedForwardDirectionBody(
				forwardDirection.x * cosAngleBody + forwardDirection.z * sinAngleBody,
				forwardDirection.y,
				forwardDirection.x * sinAngleBody - forwardDirection.z * cosAngleBody
			);
			glm::vec3 rotatedForwardDirectionHead(
				forwardDirection.x * cosAngleHead + forwardDirection.z * sinAngleHead,
				forwardDirection.y,
				forwardDirection.x * sinAngleHead - forwardDirection.z * cosAngleHead
			);



			// Miscare fata (tasta W)
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			{
				movementDirectionBody -= rotatedForwardDirectionBody; // Înainte
				movementDirectionHead -= rotatedForwardDirectionHead; // Înainte
				// Aplică viteza de mișcare
				float movementSpeed = 2.5f; // Ajustează după necesități
				glm::vec3 newPositionBody = tanks[i].Body.GetPosition() + (movementDirectionBody * movementSpeed * static_cast<float>(deltaTime));
				glm::vec3 newPositionHead = tanks[i].Head.GetPosition() + (movementDirectionBody * movementSpeed * static_cast<float>(deltaTime));
				tanks[i].Body.SetPosition(newPositionBody);
				tanks[i].Head.SetPosition(newPositionHead);
				// Rotație la stânga (tasta A)
				if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				{
					float rotationSpeed = 20.0f; // Ajustează după necesități
					glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
					rotationAngleBody += rotationSpeed * static_cast<float>(deltaTime); // Adaugă unghiul de rotație
					//rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Setează axa de rotație la axa Y
					tanks[i].Body.SetRotationAngle(rotationAngleBody);
					//tank.Body.SetRotationAxis(rotationAxis);
				}
				// Rotație la dreapta (tasta D)
				if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				{
					float rotationSpeed = 20.0f; // Ajustează după necesități
					glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
					rotationAngleBody -= rotationSpeed * static_cast<float>(deltaTime); // Adaugă unghiul de rotație
					//rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Setează axa de rotație la axa Y
					tanks[i].Body.SetRotationAngle(rotationAngleBody);
					//tank.Body.SetRotationAxis(rotationAxis);
				}
			}
			// Miscare spate (tasta S) 
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			{
				movementDirectionBody += rotatedForwardDirectionBody; // Înainte
				movementDirectionHead += rotatedForwardDirectionHead; // Înainte
				// Aplică viteza de mișcare
				float movementSpeed = 2.5f; // Ajustează după necesități
				glm::vec3 newPositionBody = tanks[i].Body.GetPosition() + (movementDirectionBody * movementSpeed * static_cast<float>(deltaTime));
				glm::vec3 newPositionHead = tanks[i].Head.GetPosition() + (movementDirectionBody * movementSpeed * static_cast<float>(deltaTime));
				tanks[i].Body.SetPosition(newPositionBody);
				tanks[i].Head.SetPosition(newPositionHead);
				// Rotație la stânga (tasta A)
				if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				{
					float rotationSpeed = 20.0f; // Ajustează după necesități
					glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
					rotationAngleBody += rotationSpeed * static_cast<float>(deltaTime); // Adaugă unghiul de rotație
					//rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Setează axa de rotație la axa Y
					tanks[i].Body.SetRotationAngle(rotationAngleBody);
					//tank.Body.SetRotationAxis(rotationAxis);
				}
				// Rotație la dreapta (tasta D)
				if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				{
					float rotationSpeed = 20.0f; // Ajustează după necesități
					glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
					rotationAngleBody -= rotationSpeed * static_cast<float>(deltaTime); // Adaugă unghiul de rotație
					//rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Setează axa de rotație la axa Y
					tanks[i].Body.SetRotationAngle(rotationAngleBody);
					//tank.Body.SetRotationAxis(rotationAxis);
				}
			}
			if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			{
				//la fel ca la tasta "A" doar ca ar trebui sa se roteasca numai tureta
				float rotationSpeed = 20.0f; // Ajustează după necesități
				glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
				rotationAngleHead += rotationSpeed * static_cast<float>(deltaTime); // Adaugă unghiul de rotație
				//rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Setează axa de rotație la axa Y
				tanks[i].Head.SetRotationAngle(rotationAngleHead);
				//tank.Body.SetRotationAxis(rotationAxis);
			}
			if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			{
				//la fel ca la tasta "D" doar ca ar trebui sa se roteasca numai tureta
				float rotationSpeed = 20.0f; // Ajustează după necesități
				glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
				rotationAngleHead -= rotationSpeed * static_cast<float>(deltaTime); // Adaugă unghiul de rotație
				//rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Setează axa de rotație la axa Y
				tanks[i].Head.SetRotationAngle(rotationAngleHead);
			}
			glm::vec3 tankPosition = tanks[i].Head.GetPosition();
			float tankRotationAngle = tanks[i].Head.GetRotationAngle();
			glm::vec3 tankRotationAxis = tanks[i].Head.GetRotationAxis();
			// Offset-ul camerei față de tank
			glm::vec3 cameraOffset = glm::vec3(0.0f, 8.0f, -20.0f); // Offset-ul inițial
			// Rotăm offset-ul camerei în funcție de rotația tankului
			cameraOffset = rotateVector(cameraOffset, glm::radians(tankRotationAngle), tankRotationAxis);
			// Poziția camerei va fi poziția tankului plus offset-ul
			glm::vec3 cameraPosition = tankPosition + cameraOffset;
			// Setăm poziția camerei
			pCamera->SetPosition(cameraPosition);
			// Orientăm camera spre tank
			pCamera->LookAt(tankPosition);
		}
	}

	////---- modificare tankIsSelected de la tastatura ---- 
	//if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
	//{
	//	tanks[1].SetIsSelected(true);
	//	/*tankIsSelected = true;*/
	//}
	//if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
	//{
	//	tanks[1].SetIsSelected(false);
	//	//tankIsSelected = false;
	//}
	

	//----------------------  CAMERA  ------------------------------------
	//if (tankIsSelected) // Daca un model este selectat de aici am setat sa fie urmarit de camera
	//{
	//	// Obținem poziția și rotația tankului
	//	glm::vec3 tankPosition = tank.Head.GetPosition();
	//	float tankRotationAngle = tank.Head.GetRotationAngle();
	//	glm::vec3 tankRotationAxis = tank.Head.GetRotationAxis();
	//	// Offset-ul camerei față de tank
	//	glm::vec3 cameraOffset = glm::vec3(0.0f, 8.0f, -20.0f); // Offset-ul inițial
	//	// Rotăm offset-ul camerei în funcție de rotația tankului
	//	cameraOffset = rotateVector(cameraOffset, glm::radians(tankRotationAngle), tankRotationAxis);
	//	// Poziția camerei va fi poziția tankului plus offset-ul
	//	glm::vec3 cameraPosition = tankPosition + cameraOffset;
	//	// Setăm poziția camerei
	//	pCamera->SetPosition(cameraPosition);
	//	// Orientăm camera spre tank
	//	pCamera->LookAt(tankPosition);
	//}
	//---- Putem misca din sageti camera numai daca nu avem un model selectat
	if (!IsAnObjectSelected(tanks))
	{
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			pCamera->ProcessKeyboard(Camera::FORWARD, (float)deltaTime * 5);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			pCamera->ProcessKeyboard(Camera::BACKWARD, (float)deltaTime * 5);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			pCamera->ProcessKeyboard(Camera::LEFT, (float)deltaTime * 5);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
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
	if (!tankIsSelected) // am facut aceasta conditie ca sa nu putem modifica deloc camera cu mouse-ul atunci cand un model este selectat
		pCamera->MouseControl((float)xpos, (float)ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yOffset)
{
	pCamera->ProcessMouseScroll((float)yOffset);
}
