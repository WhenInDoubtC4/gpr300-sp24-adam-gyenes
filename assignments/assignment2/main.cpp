#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <ew/shader.h>
#include <ew/model.h>
#include <ew/camera.h>
#include <ew/cameraController.h>
#include <ew/transform.h>
#include <ew/texture.h>
#include <ew/procGen.h>

#include <util/model.h>
#include <util/framebuffer.h>
#include <util/shader.h>

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();

//Global state
constexpr glm::vec3 CAMERA_INIT_POSITION = glm::vec3(0.f, 0.f, 5.f);
constexpr glm::vec3 CAMERA_INIT_TARGET = glm::vec3(0.f);

int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

ew::Camera camera;
ew::CameraController cameraController;
float cameraFov = 60.f;

struct Material
{
	float ambientStrength = 0.6f;
	float diffuseStrength = 1.f;
	float specularStrength = 1.f;
	float shininess = 128;
};
float shadowBrightness = 0.3;
float shadowMinBias = 0.005f;
float shadowMaxBias = 0.015f;

Material material;
glm::vec3 lightColor = glm::vec3(1.0);
glm::vec3 ambientColor = glm::vec3(0.3, 0.4, 0.45);

GLuint currentColorTexture;
GLuint currentNormalTexture;
int currentTexture = 0;
int prevTexture = 0;
GLuint brickColorTexture;
GLuint brickNormalTexture;
GLuint rockColorTexture;
GLuint rockNormalTexture;

Util::Framebuffer postprocessFramebuffer;
bool boxBlurEnable = false;
int boxBlurSize = 2;

bool chromaticAberrationEnable = false;
float chromaticAberrationSize = 0.09;
glm::vec2 focusPoint(0.5f, 0.5f);

bool dofEnable = false;
int dofBlurSize = 8;
float dofMinDistance = 1.0;
float dofMaxDistance = 3.0;

Util::Framebuffer shadowFramebuffer;
//Light that's a camera in disguise
ew::Camera directionalLight;

void resetCamera(ew::Camera* camera, ew::CameraController* controller)
{
	cameraFov = 60.f;
	camera->position = CAMERA_INIT_POSITION;
	camera->target = CAMERA_INIT_TARGET;
	camera->fov = cameraFov;
	controller->yaw = 0.f;
	controller->pitch = 0.f;
}

void createPostprocessFramebuffer(int width, int height)
{
	postprocessFramebuffer = Util::Framebuffer(glm::vec2(width, height));
	postprocessFramebuffer.addColorAttachment();
	postprocessFramebuffer.addDepthAttachment();
}

void startRenderSceneToFramebuffer(const Util::Framebuffer& framebuffer)
{
	if (!framebuffer.isComplete())
	{
		printf("Attempt to render to an incomplete framebuffer\n");
		return;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.getFBO());
	glViewport(0, 0, framebuffer.getSize().x, framebuffer.getSize().y);
	glClearColor(0.6f, 0.8f, 0.92f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int main() {
	GLFWwindow* window = initWindow("Assignment 2", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	createPostprocessFramebuffer(screenWidth, screenHeight);
	
	//Create shadow framebuffer
	shadowFramebuffer = Util::Framebuffer(glm::vec2(2048));
	glBindTexture(GL_TEXTURE_2D, shadowFramebuffer.addDepthAttachment());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float boderColor[4] = { 1.f, 1.f, 1.f, 1.f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, boderColor);

	camera.position = CAMERA_INIT_POSITION;
	camera.target = CAMERA_INIT_TARGET;
	camera.aspectRatio = float(screenWidth) / float(screenHeight);
	camera.fov = cameraFov;

	directionalLight.orthographic = true;
	directionalLight.position = glm::vec3(-1.f, 4.f, 7.f);

	Util::Shader depthOnlyShader("assets/depthOnly.vert", "assets/depthOnly.frag");
	Util::Shader shader("assets/lit.vert", "assets/lit.frag");
	Util::Shader postprocessShader("assets/postprocess.vert", "assets/postprocess.frag");
	Util::Model monkeyModel("assets/Suzanne.obj");
	ew::Transform monkeyTransform;
	//Using a basic plane mesh from Maya since procGen doesn't calculate TBN
	Util::Model planeModel("assets/plane.fbx");
	ew::Transform planeTransform;
	planeTransform.position.z = -2.5;
	planeTransform.position.y = -2.f;

	brickColorTexture = ew::loadTexture("assets/brick2_color.jpg");
	brickNormalTexture = ew::loadTexture("assets/brick2_normal.jpg");

	rockColorTexture = ew::loadTexture("assets/rock_color.jpg");
	rockNormalTexture = ew::loadTexture("assets/rock_normal.jpg");

	currentColorTexture = brickColorTexture;
	currentNormalTexture = brickNormalTexture;

	//Create dummy VAO for screen
	GLuint screenVAO;
	glCreateVertexArrays(1, &screenVAO);

	glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;		

		cameraController.move(window, &camera, deltaTime);
		camera.fov = cameraFov;

		monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.f, 1.f, 0.f));
		glBindTextureUnit(0, shadowFramebuffer.getDepthAttachment());
		glBindTextureUnit(1, currentColorTexture);
		glBindTextureUnit(2, currentNormalTexture);

		//Render to shadow map
		glCullFace(GL_FRONT);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer.getFBO());
		glViewport(0, 0, shadowFramebuffer.getSize().x, shadowFramebuffer.getSize().y);
		glClear(GL_DEPTH_BUFFER_BIT);

		glm::mat4 lightView = glm::lookAt(directionalLight.position, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
		glm::mat4 lightMatrix = directionalLight.projectionMatrix() * lightView;
		depthOnlyShader.use();
		depthOnlyShader.setMat4("_view", lightMatrix);
		depthOnlyShader.setMat4("_model", monkeyTransform.modelMatrix());
		monkeyModel.draw();
		depthOnlyShader.setMat4("_model", planeTransform.modelMatrix());
		planeModel.draw();

		//Render to color buffer
		glCullFace(GL_BACK);
		startRenderSceneToFramebuffer(postprocessFramebuffer);

		shader.use();
		shader.setMat4("_model", monkeyTransform.modelMatrix());
		monkeyModel.draw();
		shader.setMat4("_model", planeTransform.modelMatrix());
		planeModel.draw();
		shader.setMat4("_viewProjection", camera.projectionMatrix() * camera.viewMatrix());
		shader.setMat4("_lightViewProjection", lightMatrix);
		shader.setVec3("_cameraPosition", camera.position);
		shader.setVec3("_lightPosition", directionalLight.position);
		shader.setFloat("_shadowBrightness", shadowBrightness);
		shader.setFloat("_shadowMinBias", shadowMinBias);
		shader.setFloat("_shadowMaxBias", shadowMaxBias);
		shader.setInt("_shadowMap", 0);
		shader.setInt("_mainTex", 1);
		shader.setInt("_normalTex", 2);
		shader.setVec3("_ambientColor", ambientColor);
		shader.setVec3("_lightColor", lightColor);
		shader.setFloat("_material.ambientStrength", material.ambientStrength);
		shader.setFloat("_material.diffuseStrength", material.diffuseStrength);
		shader.setFloat("_material.specularStrength", material.specularStrength);
		shader.setFloat("_material.shininess", material.shininess);

		//Render to screen (default) fbo
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindTextureUnit(0, postprocessFramebuffer.getColorAttachment());
		glBindTextureUnit(1, postprocessFramebuffer.getDepthAttachment());
		postprocessShader.use();

		postprocessShader.setSubroutine(GL_FRAGMENT_SHADER, {
			std::make_pair("_blurFunction", boxBlurEnable ? "blurEnabled" : "blurDisabled"),
			std::make_pair("_dofFunction", dofEnable ? "dofEnabled" : "dofDisabled"),
			std::make_pair("_chromaticAberrationFunction", chromaticAberrationEnable ? "chromaticAberrationEnabled" : "chromaticAberrationDisabled") });

		postprocessShader.setInt("_colorBuffer", 0);
		postprocessShader.setInt("_depthBuffer", 1);
		postprocessShader.setVec2("_focusPoint", focusPoint);
		postprocessShader.setInt("_boxBlurSize", boxBlurSize);
		postprocessShader.setFloat("_chromaticAberrationSize", chromaticAberrationSize);
		postprocessShader.setFloat("_dofMinDistance", dofMinDistance);
		postprocessShader.setFloat("_dofMaxDistance", dofMaxDistance);
		postprocessShader.setInt("_dofBlurSize", dofBlurSize);

		glBindVertexArray(screenVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		drawUI();

		glfwSwapBuffers(window);
	}
	printf("Shutting down...");
}

void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	if (ImGui::CollapsingHeader("Light"))
	{
		ImGui::ColorEdit3("Ambient color", &ambientColor[0], ImGuiColorEditFlags_Float);
		ImGui::ColorEdit3("Light color", &lightColor[0], ImGuiColorEditFlags_Float);
		ImGui::DragFloat3("Light position", &directionalLight.position.x, 0.1f, -10.f, 10.f);
		ImGui::SliderFloat("Shadow brightness", &shadowBrightness, 0.f, 1.f);
		ImGui::DragFloat("Shadow min bias", &shadowMinBias, 0.001f);
		ImGui::DragFloat("Shadow max bias", &shadowMaxBias, 0.001f);
		ImGui::SliderFloat("Shadow frustum size", &directionalLight.orthoHeight, 0.f, 20.f);
	}
	if (ImGui::CollapsingHeader("Material"))
	{
		ImGui::Combo("Texture", &currentTexture, "Brick\0Rock");
		if (currentTexture != prevTexture)
		{
			prevTexture = currentTexture;
			if (currentTexture == 0)
			{
				currentColorTexture = brickColorTexture;
				currentNormalTexture = brickNormalTexture;
			}
			else if (currentTexture == 1)
			{
				currentColorTexture = rockColorTexture;
				currentNormalTexture = rockNormalTexture;
			}
		}

		ImGui::SliderFloat("Ambient strength", &material.ambientStrength, 0.f, 1.f);
		ImGui::SliderFloat("Diffuse strength", &material.diffuseStrength, 0.f, 1.f);
		ImGui::SliderFloat("Specular strength", &material.specularStrength, 0.f, 1.f);
		ImGui::DragFloat("Shininess", &material.shininess, 1.f, 0.f, 1024.f);
	}
	if (ImGui::CollapsingHeader("Camera"))
	{
		ImGui::SliderFloat("FOV", &cameraFov, 20.f, 180.f);
		if (ImGui::Button("Reset"))
		{
			resetCamera(&camera, &cameraController);
		}
	}
	if (ImGui::CollapsingHeader("Post processing"))
	{
		ImGui::SliderFloat2("Focus point", &focusPoint[0], 0.0f, 1.f);

		ImGui::Indent();
		if (ImGui::CollapsingHeader("Box blur"))
		{
			ImGui::Checkbox("Enable##1", &boxBlurEnable);
			ImGui::DragInt("Size##1", &boxBlurSize, 0.1f, 0, 99);
		}
		if (ImGui::CollapsingHeader("Depth of field"))
		{
			ImGui::Checkbox("Enable##2", &dofEnable);
			ImGui::DragInt("Blur size", &dofBlurSize, 0.1f, 1, 99);
			ImGui::DragFloat("Min distance", &dofMinDistance, 0.1f);
			ImGui::DragFloat("Max distance", &dofMaxDistance, 0.1f);
		}
		if (ImGui::CollapsingHeader("Chromatic aberration"))
		{
			ImGui::Checkbox("Enable##3", &chromaticAberrationEnable);
			ImGui::SliderFloat("Size##2", &chromaticAberrationSize, 0.0f, 1.f);
		}
		ImGui::Unindent();
	}
	ImGui::End();

	ImGui::Begin("Shadow map");
	ImGui::BeginChild("Shadow map");
	ImVec2 windowSize = ImGui::GetWindowSize();
	ImGui::Image(reinterpret_cast<ImTextureID>(shadowFramebuffer.getDepthAttachment()), windowSize, ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));
	ImGui::EndChild();
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;

	//Generate new framebuffer with updated size
	createPostprocessFramebuffer(screenWidth, screenHeight);
}

/// <summary>
/// Initializes GLFW, GLAD, and IMGUI
/// </summary>
/// <param name="title">Window title</param>
/// <param name="width">Window width</param>
/// <param name="height">Window height</param>
/// <returns>Returns window handle on success or null on fail</returns>
GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}

