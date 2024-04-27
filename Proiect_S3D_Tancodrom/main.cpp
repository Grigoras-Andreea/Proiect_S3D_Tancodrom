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
	const float hoursPerDay = 24.0f;
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

void RenderModels(Shader& lightingShader, Model& tank1, Model& tank2, Model& tank3, Model& tank4, Model& tank5, Model& tank6, Model& tank7, Model& tank8,
	Model& mountain1, Model& mountain2, Model& mountain3, Model& mountain4,
	Helicopter& helicopter1, Helicopter& helicopter2,
	Model& cloud1, Model& cloud2, Model& cloud3, Model& cloud4, Model& cloud5, Model& cloud6, Model& cloud7, Model& cloud8, Model& cloud9,
	Model& cloud10, Model& cloud11, Model& cloud12, Model& cloud13, Model& cloud14, Model& cloud15, Model& cloud16, Model& cloud17,
	Model& cloud18, Model& cloud19, Model& cloud20, Model& cloud21, Model& cloud22, Model& cloud23, Model& cloud24, Model& cloud25
)
{
	glBindTexture(GL_TEXTURE_2D, 2);
	lightingShader.SetMat4("model", mountain1.GetModelMatrix());
	mountain1.Draw(lightingShader);
	lightingShader.SetMat4("model", mountain2.GetModelMatrix());
	mountain2.Draw(lightingShader);
	lightingShader.SetMat4("model", mountain3.GetModelMatrix());
	mountain3.Draw(lightingShader);
	lightingShader.SetMat4("model", mountain4.GetModelMatrix());
	mountain4.Draw(lightingShader);


	glBindTexture(GL_TEXTURE_2D, 1);
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
	lightingShader.SetMat4("model", cloud1.GetModelMatrix());
	cloud1.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud2.GetModelMatrix());
	cloud2.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud3.GetModelMatrix());
	cloud3.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud4.GetModelMatrix());
	cloud4.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud5.GetModelMatrix());
	cloud5.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud6.GetModelMatrix());
	cloud6.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud7.GetModelMatrix());
	cloud7.Draw(lightingShader);
	//---------------------
	glBindTexture(GL_TEXTURE_2D, 4);
	lightingShader.SetMat4("model", cloud8.GetModelMatrix());
	cloud8.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud9.GetModelMatrix());
	cloud9.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud10.GetModelMatrix());
	cloud10.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud11.GetModelMatrix());
	cloud11.Draw(lightingShader);
	//---------------------
	glBindTexture(GL_TEXTURE_2D, 4);
	lightingShader.SetMat4("model", cloud12.GetModelMatrix());
	cloud12.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud13.GetModelMatrix());
	cloud13.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud14.GetModelMatrix());
	cloud14.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud15.GetModelMatrix());
	cloud15.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud16.GetModelMatrix());
	cloud16.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud17.GetModelMatrix());
	cloud17.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud18.GetModelMatrix());
	cloud18.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud19.GetModelMatrix());
	cloud19.Draw(lightingShader);
	//---------------------
	glBindTexture(GL_TEXTURE_2D, 4);
	lightingShader.SetMat4("model", cloud20.GetModelMatrix());
	cloud20.Draw(lightingShader);
	//---------------------
	glBindTexture(GL_TEXTURE_2D, 4);
	lightingShader.SetMat4("model", cloud21.GetModelMatrix());
	cloud21.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud22.GetModelMatrix());
	cloud22.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud23.GetModelMatrix());
	cloud23.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud24.GetModelMatrix());
	cloud24.Draw(lightingShader);
	lightingShader.SetMat4("model", cloud25.GetModelMatrix());
	cloud25.Draw(lightingShader);
}


void PozitionateModels(Model& tank1, Model& tank2, Model& tank3, Model& tank4, Model& tank5, Model& tank6, Model& tank7, Model& tank8,
	Model& mountain1, Model& mountain2, Model& mountain3, Model& mountain4,
	Helicopter& helicopter1, Helicopter& helicopter2,
	Model& cloud1, Model& cloud2, Model& cloud3, Model& cloud4, Model& cloud5, Model& cloud6, Model& cloud7, Model& cloud8, Model& cloud9,
	Model& cloud10, Model& cloud11, Model& cloud12, Model& cloud13, Model& cloud14, Model& cloud15, Model& cloud16, Model& cloud17,
	Model& cloud18, Model& cloud19, Model& cloud20, Model& cloud21, Model& cloud22, Model& cloud23, Model& cloud24, Model& cloud25
)
{
	//---- Tancuri ----
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
	tank6.SetPosition(glm::vec3(-12.0f, 0.0f, -25.0f));
	tank6.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	tank7.SetPosition(glm::vec3(-25.0f, 0.0f, -29.0f));
	tank7.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	tank8.SetPosition(glm::vec3(14.0f, 0.0f, -30.0f));
	tank8.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));


	//---- Munti ----
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


	//---- Elicoptere ----
	helicopter1.Body.SetPosition(glm::vec3(20.25f, 20.0f, -30.0f));
	helicopter1.Body.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	//helicopter1.SetRotationAngle(180.0f);

	helicopter1.ProppelerUp.SetPosition(glm::vec3(20.0f, 20.0f, -30.0f));
	helicopter1.ProppelerUp.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));

	helicopter1.ProppelerBack.SetPosition(glm::vec3(20.5f, 23.63f, -41.27f));
	helicopter1.ProppelerBack.SetRotationAxis(glm::vec3(1.0f, 0.0f, 0.0f));

	helicopter2.Body.SetPosition(glm::vec3(19.75f, 20.0f, 30.0f));
	helicopter2.Body.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	helicopter2.Body.SetRotationAngle(180.0f);

	helicopter2.ProppelerUp.SetPosition(glm::vec3(20.0f, 20.0f, 30.0f));
	helicopter2.ProppelerUp.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	//helicopter2_elice.SetRotationAngle(180.0f);

	helicopter2.ProppelerBack.SetPosition(glm::vec3(19.5f, 23.63f, 41.27f));
	helicopter2.ProppelerBack.SetRotationAxis(glm::vec3(1.0f, 0.0f, 0.0f));
	//helicopter2_elice_spate.SetRotationAngle(180.0f);

	//---- Nori ----
	cloud1.SetPosition(glm::vec3(30.0f, 170.0f, 210.0f));
	cloud1.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud1.SetRotationAngle(180.0f);
	cloud1.SetScale(glm::vec3(0.35f));

	cloud2.SetPosition(glm::vec3(-70.0f, 170.0f, 50.0f));
	cloud2.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud2.SetRotationAngle(180.0f);
	cloud2.SetScale(glm::vec3(0.2f));

	cloud3.SetPosition(glm::vec3(85.0f, 170.0f, -35.0f));
	cloud3.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud3.SetRotationAngle(180.0f);
	cloud3.SetScale(glm::vec3(0.55f));

	cloud4.SetPosition(glm::vec3(20.0f, 170.0f, 205.0f));
	cloud4.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud4.SetRotationAngle(180.0f);
	cloud4.SetScale(glm::vec3(0.35f));

	cloud5.SetPosition(glm::vec3(95.0f, 170.0f, -100.0f));
	cloud5.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud5.SetRotationAngle(180.0f);
	cloud5.SetScale(glm::vec3(0.45f));

	cloud6.SetPosition(glm::vec3(-50.0f, 170.0f, -90.0f));
	cloud6.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud6.SetRotationAngle(180.0f);
	cloud6.SetScale(glm::vec3(0.35f));

	cloud7.SetPosition(glm::vec3(120.0f, 170.0f, 65.0f));
	cloud7.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud7.SetRotationAngle(180.0f);
	cloud7.SetScale(glm::vec3(0.25f));

	//---- Nori2 ----

	cloud8.SetPosition(glm::vec3(270.0f, 150.0f, -130.0f));
	cloud8.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud8.SetRotationAngle(180.0f);
	cloud8.SetScale(glm::vec3(0.65f));

	cloud9.SetPosition(glm::vec3(285.0f, 140.0f, -40.0f));
	cloud9.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud9.SetRotationAngle(180.0f);
	cloud9.SetScale(glm::vec3(0.25f));

	cloud10.SetPosition(glm::vec3(85.0f, 170.0f, 135.0f));
	cloud10.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud10.SetRotationAngle(180.0f);
	cloud10.SetScale(glm::vec3(0.45f));

	cloud11.SetPosition(glm::vec3(-220.0f, 160.0f, -125.0f));
	cloud11.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud11.SetRotationAngle(180.0f);
	cloud11.SetScale(glm::vec3(0.25f));

	//---- Nori3 ----
	cloud12.SetPosition(glm::vec3(295.0f, 150.0f, 100.0f));
	cloud12.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud12.SetRotationAngle(180.0f);
	cloud12.SetScale(glm::vec3(0.25f));

	cloud13.SetPosition(glm::vec3(250.0f, 170.0f, 90.0f));
	cloud13.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud13.SetRotationAngle(180.0f);
	cloud13.SetScale(glm::vec3(0.25f));

	cloud14.SetPosition(glm::vec3(-120.0f, 140.0f, 65.0f));
	cloud14.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud14.SetRotationAngle(180.0f);
	cloud14.SetScale(glm::vec3(0.55f));

	cloud15.SetPosition(glm::vec3(-270.0f, 140.0f, 10.0f));
	cloud15.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud15.SetRotationAngle(180.0f);
	cloud15.SetScale(glm::vec3(0.45f));

	//---- Nori4 ----
	cloud16.SetPosition(glm::vec3(-130.0f, 160.0f, 250.0f));
	cloud16.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud16.SetRotationAngle(180.0f);
	cloud16.SetScale(glm::vec3(0.40f));

	cloud17.SetPosition(glm::vec3(150.0f, 170.0f, -250.0f));
	cloud17.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud17.SetRotationAngle(180.0f);
	cloud17.SetScale(glm::vec3(0.35f));

	cloud18.SetPosition(glm::vec3(-20.0f, 150.0f, 10.0f));
	cloud18.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud18.SetRotationAngle(180.0f);
	cloud18.SetScale(glm::vec3(0.45f));

	cloud19.SetPosition(glm::vec3(-50.0f, 150.0f, -230.0f));
	cloud19.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud19.SetRotationAngle(180.0f);
	cloud19.SetScale(glm::vec3(0.35f));

	//---- Nori5 ----
	cloud20.SetPosition(glm::vec3(-250.0f, 150.0f, -130.0f));
	cloud20.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud20.SetRotationAngle(180.0f);
	cloud20.SetScale(glm::vec3(0.85f));

	//---- Nori6 ----
	cloud21.SetPosition(glm::vec3(250.0f, 150.0f, -130.0f));
	cloud21.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud21.SetRotationAngle(180.0f);
	cloud21.SetScale(glm::vec3(0.85f));

	cloud22.SetPosition(glm::vec3(250.0f, 150.0f, 160.0f));
	cloud22.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud22.SetRotationAngle(180.0f);
	cloud22.SetScale(glm::vec3(0.85f));

	cloud23.SetPosition(glm::vec3(-150.0f, 150.0f, 130.0f));
	cloud23.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud23.SetRotationAngle(180.0f);
	cloud23.SetScale(glm::vec3(0.85f));

	cloud24.SetPosition(glm::vec3(-250.0f, 150.0f, 130.0f));
	cloud24.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud24.SetRotationAngle(180.0f);
	cloud24.SetScale(glm::vec3(0.85f));

	cloud25.SetPosition(glm::vec3(250.0f, 150.0f, -160.0f));
	cloud25.SetRotationAxis(glm::vec3(0.0f, 1.0f, 0.0f));
	cloud25.SetRotationAngle(180.0f);
	cloud25.SetScale(glm::vec3(0.85f));

}

void moveClouds(
	Model& cloud1, Model& cloud2, Model& cloud3, Model& cloud4, Model& cloud5, Model& cloud6, Model& cloud7, Model& cloud8, Model& cloud9,
	Model& cloud10, Model& cloud11, Model& cloud12, Model& cloud13, Model& cloud14, Model& cloud15, Model& cloud16, Model& cloud17,
	Model& cloud18, Model& cloud19, Model& cloud20, Model& cloud21, Model& cloud22, Model& cloud23, Model& cloud24, Model& cloud25
)
{
	if (cloud15.GetPosition().x > -400.0f && cloud17.GetPosition().x > -400.0f)
	{
		//move clouds
		cloud1.SetPosition(cloud1.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud2.SetPosition(cloud2.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud3.SetPosition(cloud3.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud4.SetPosition(cloud4.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud5.SetPosition(cloud5.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud6.SetPosition(cloud6.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud7.SetPosition(cloud7.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud8.SetPosition(cloud8.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud9.SetPosition(cloud9.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud10.SetPosition(cloud10.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud11.SetPosition(cloud11.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud12.SetPosition(cloud12.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud13.SetPosition(cloud13.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud14.SetPosition(cloud14.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud15.SetPosition(cloud15.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud16.SetPosition(cloud16.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud17.SetPosition(cloud17.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud18.SetPosition(cloud18.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud19.SetPosition(cloud19.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud20.SetPosition(cloud20.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud21.SetPosition(cloud21.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud22.SetPosition(cloud22.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud23.SetPosition(cloud23.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud24.SetPosition(cloud24.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
		cloud25.SetPosition(cloud25.GetPosition() + glm::vec3(-0.001f, 0.0f, -0.001f));
	}

}

bool isNight = true;

void rotate_elice(Model& helicpter3_elice, double deltaTime)
{
	helicpter3_elice.SetRotationAngle(helicpter3_elice.GetRotationAngle() + 100.0f * deltaTime * 250);
}

void rotate_elice_spate(Model& helicpter3_elice_spate, double deltaTime)
{
	helicpter3_elice_spate.SetRotationAngle(helicpter3_elice_spate.GetRotationAngle() + 100.0f * deltaTime * 50);
}

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

	Model helicopter_body(heli_bodyObjFileName, false);
	Model helicopter_elice(heli_eliceObjFileName, false);
	Model helicopter_elice_spate(heli_elice_spateObjFileName, false);

	Helicopter helicopter1(helicopter_body, helicopter_elice, helicopter_elice_spate);
	Helicopter helicopter2(helicopter_body, helicopter_elice, helicopter_elice_spate);

	Model cloud1(cloudObjFileName, false);
	Model cloud2(cloudObjFileName, false);
	Model cloud3(cloudObjFileName, false);
	Model cloud4(cloudObjFileName, false);
	Model cloud5(cloudObjFileName, false);
	Model cloud6(cloudObjFileName, false);
	Model cloud7(cloudObjFileName, false);

	Model cloud8(cloud2ObjFileName, false);
	Model cloud9(cloud2ObjFileName, false);
	Model cloud10(cloud2ObjFileName, false);
	Model cloud11(cloud2ObjFileName, false);

	Model cloud12(cloud3ObjFileName, false);
	Model cloud13(cloud3ObjFileName, false);
	Model cloud14(cloud3ObjFileName, false);
	Model cloud15(cloud3ObjFileName, false);
	Model cloud16(cloud3ObjFileName, false);
	Model cloud17(cloud3ObjFileName, false);
	Model cloud18(cloud3ObjFileName, false);
	Model cloud19(cloud3ObjFileName, false);

	Model cloud20(cloud4ObjFileName, false);

	Model cloud21(cloud5ObjFileName, false);
	Model cloud22(cloud5ObjFileName, false);
	Model cloud23(cloud5ObjFileName, false);
	Model cloud24(cloud5ObjFileName, false);
	Model cloud25(cloud5ObjFileName, false);


	unsigned int floorTexture = CreateTexture(std::string(currentPathChr) + "\\ColoredFloor.png");
	unsigned int mountainTexture = CreateTexture(std::string(currentPathChr) + "\\Models\\mountain\\ground_grass_3264_4062_Small.jpg");
	unsigned int cloudTexture = CreateTexture(std::string(currentPathChr) + "\\Models\\clouds\\white.jpg");

	float radius = 150.0f; // Raza cercului pe care se va rota lumina
	float speed = 0.26f;

	//tankuri.push_back(tank1);
	tankuri.push_back(tank2);
	tankuri.push_back(tank3);
	tankuri.push_back(tank4);


	PozitionateModels(
		tank1, tank2, tank3, tank4, tank5, tank6, tank7, tank8,
		mountain1, mountain2, mountain3, mountain4,
		helicopter1, helicopter2,
		cloud1, cloud2, cloud3, cloud4, cloud5, cloud6, cloud7, cloud8, cloud9, cloud10,
		cloud11, cloud12, cloud13, cloud14, cloud15, cloud16, cloud17, cloud18, cloud19,
		cloud20, cloud21, cloud22, cloud23, cloud24, cloud25
	);

	// Render loop
	while (!glfwWindowShouldClose(window)) {
		// Per-frame time logic
		double currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		moveClouds(
			cloud1, cloud2, cloud3, cloud4, cloud5, cloud6, cloud7, cloud8, cloud9,
			cloud10, cloud11, cloud12, cloud13, cloud14, cloud15, cloud16, cloud17,
			cloud18, cloud19, cloud20, cloud21, cloud22, cloud23, cloud24, cloud25
		);

		rotate_elice(helicopter1.ProppelerUp, deltaTime);
		rotate_elice(helicopter2.ProppelerUp, deltaTime);

		rotate_elice_spate(helicopter1.ProppelerBack, deltaTime);
		rotate_elice_spate(helicopter2.ProppelerBack, deltaTime);

		// Input
		//processInput(window);
		processInput(window, tank1);		// aici ar cam trb modificat pentru a putea controla toate tancurile, acum se poate controla doar tancul tank1

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

		// asta a fost pentru modificarea culorii Shaderelor PhongLight
		if (lightZ >= 0) {
			//lightingShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);
			//lightingShader.SetVec3("objectColor", 0.5f, 1.0f, 0.3f);
		}
		else {
			//lightingShader.SetVec3("lightColor", 0.2f, 0.2f, 0.2f);
			//lightingShader.SetVec3("objectColor", 0.8f, 0.8f, 0.3f);
			lightPos.x = -lightPos.x;
		}

		lightingShader.SetVec3("lightPos", lightPos);
		//glBindTexture(GL_TEXTURE_2D, floorTexture);
		lightingShader.SetVec3("viewPos", pCamera->GetPosition());

		lightingShader.SetMat4("projection", pCamera->GetProjectionMatrix());
		lightingShader.SetMat4("view", pCamera->GetViewMatrix());


		// Set model matrix and draw the model
		RenderModels(
			lightingShader, tank1, tank2, tank3, tank4, tank5, tank6, tank7, tank8,
			mountain1, mountain2, mountain3, mountain4,
			helicopter1, helicopter2,
			cloud1, cloud2, cloud3, cloud4, cloud5, cloud6, cloud7, cloud8, cloud9, cloud10,
			cloud11, cloud12, cloud13, cloud14, cloud15, cloud16, cloud17, cloud18, cloud19,
			cloud20, cloud21, cloud22, cloud23, cloud24, cloud25
		);


		// Use lamp shader
		lampShader.Use();
		lampShader.SetMat4("projection", pCamera->GetProjectionMatrix());
		lampShader.SetMat4("view", pCamera->GetViewMatrix());

		// Set light model matrix and draw the lamp object
		glm::mat4 lightModel = glm::translate(glm::mat4(1.0), lightPos);
		lightModel = glm::scale(lightModel, glm::vec3(0.5f)); // a smaller cube
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

glm::vec3 rotateVector(const glm::vec3& vec, float angle, const glm::vec3& axis) {
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, axis);
	return glm::vec3(rotationMatrix * glm::vec4(vec, 1.0f));
}

bool tankIsSelected = false;
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window, Model& piratObjModel)
{
	//---- EXIT APP ----
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	//---- Control miscari model(tank)
	if (tankIsSelected) {
		glm::vec3 movementDirection(0.0f);
		float rotationAngle = piratObjModel.GetRotationAngle();

		// Calculul direcției de mișcare în funcție de unghiul de rotație
		float radianRotationAngle = glm::radians(-rotationAngle); // Convertim unghiul în radiani
		float cosAngle = cos(radianRotationAngle);
		float sinAngle = sin(radianRotationAngle);

		glm::vec3 forwardDirection(0.0f, 0.0f, 1.0f);
		glm::vec3 rotatedForwardDirection(
			forwardDirection.x * cosAngle + forwardDirection.z * sinAngle,
			forwardDirection.y,
			forwardDirection.x * sinAngle - forwardDirection.z * cosAngle
		);



		// Miscare fata (tasta W)
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			movementDirection -= rotatedForwardDirection; // Înainte
			// Aplică viteza de mișcare
			float movementSpeed = 1.5f; // Ajustează după necesități
			glm::vec3 newPosition = piratObjModel.GetPosition() + (movementDirection * movementSpeed * static_cast<float>(deltaTime));
			piratObjModel.SetPosition(newPosition);
			// Rotație la stânga (tasta A)
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			{
				float rotationSpeed = 20.0f; // Ajustează după necesități
				glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
				rotationAngle += rotationSpeed * static_cast<float>(deltaTime); // Adaugă unghiul de rotație
				rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Setează axa de rotație la axa Y
				piratObjModel.SetRotationAngle(rotationAngle);
				piratObjModel.SetRotationAxis(rotationAxis);
			}
			// Rotație la dreapta (tasta D)
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			{
				float rotationSpeed = 20.0f; // Ajustează după necesități
				glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
				rotationAngle -= rotationSpeed * static_cast<float>(deltaTime); // Scade unghiul de rotație
				rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Setează axa de rotație la axa Y
				piratObjModel.SetRotationAngle(rotationAngle);
				piratObjModel.SetRotationAxis(rotationAxis);
			}
		}
		// Miscare spate (tasta S) 
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		{
			movementDirection += rotatedForwardDirection; // Înapoi
			// Aplică viteza de mișcare
			float movementSpeed = 1.5f; // Ajustează după necesități
			glm::vec3 newPosition = piratObjModel.GetPosition() + (movementDirection * movementSpeed * static_cast<float>(deltaTime));
			piratObjModel.SetPosition(newPosition);
			// Rotație la stânga (tasta A)
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			{
				float rotationSpeed = 20.0f; // Ajustează după necesități
				glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
				rotationAngle += rotationSpeed * static_cast<float>(deltaTime); // Adaugă unghiul de rotație
				rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Setează axa de rotație la axa Y
				piratObjModel.SetRotationAngle(rotationAngle);
				piratObjModel.SetRotationAxis(rotationAxis);
			}
			// Rotație la dreapta (tasta D)
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			{
				float rotationSpeed = 20.0f; // Ajustează după necesități
				glm::vec3 rotationAxis(0.0f, 1.0f, 0.0f);
				rotationAngle -= rotationSpeed * static_cast<float>(deltaTime); // Scade unghiul de rotație
				rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Setează axa de rotație la axa Y
				piratObjModel.SetRotationAngle(rotationAngle);
				piratObjModel.SetRotationAxis(rotationAxis);
			}
		}
	}

	//---- modificare tankIsSelected de la tastatura ---- 
	if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
		tankIsSelected = true;
	if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
		tankIsSelected = false;


	//----------------------  CAMERA  ------------------------------------
	if (tankIsSelected) // Daca un model este selectat de aici am setat sa fie urmarit de camera
	{
		// Obținem poziția și rotația tankului
		glm::vec3 tankPosition = piratObjModel.GetPosition();
		float tankRotationAngle = piratObjModel.GetRotationAngle();
		glm::vec3 tankRotationAxis = piratObjModel.GetRotationAxis();
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
	//---- Putem misca din sageti camera numai daca nu avem un model selectat
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
	if (!tankIsSelected) // am facut aceasta conditie ca sa nu putem modifica deloc camera cu mouse-ul atunci cand un model este selectat
		pCamera->MouseControl((float)xpos, (float)ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yOffset)
{
	pCamera->ProcessMouseScroll((float)yOffset);
}
