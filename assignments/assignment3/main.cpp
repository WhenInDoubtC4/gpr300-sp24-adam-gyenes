#include <stdio.h>
#include <math.h>
#include <map>

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

#include "mainGlobals.h"

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();

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

	//GBuffer (with 3 color attachments + depth)
	gBuffer = Util::Framebuffer(glm::vec2(width, height));
	gBuffer.addColorAttachment(GL_RGB32F); //World position
	gBuffer.addColorAttachment(GL_RGB16F); //World normal
	gBuffer.addColorAttachment(GL_RGB16F); //Albedo
	gBuffer.addColorAttachment(GL_RGB16F); //Shading model
	gBuffer.addDepthAttachment(); //Depth buffer
	if (!gBuffer.isComplete()) printf("ERROR: G-buffer is not complete!\n");

	camera.aspectRatio = float(width) / float(height);
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

void setupScene()
{
	//Main camera and lighting
	camera.position = CAMERA_INIT_POSITION;
	camera.target = CAMERA_INIT_TARGET;
	camera.aspectRatio = float(screenWidth) / float(screenHeight);
	camera.fov = cameraFov;

	directionalLight.orthographic = true;
	directionalLight.position = glm::vec3(-1.f, 4.f, 7.f);

	//Shadow buffer
	shadowFramebuffer = Util::Framebuffer(glm::vec2(2048));
	glBindTexture(GL_TEXTURE_2D, shadowFramebuffer.addDepthAttachment());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float boderColor[4] = { 1.f, 1.f, 1.f, 1.f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, boderColor);

	//Point light UBO
	glGenBuffers(1, &lightsUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, lightsUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(pointLights), pointLights, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, lightsUBOBinding, lightsUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	//Shader setup
	gBufferShader = new Util::Shader("assets/lit.vert", "assets/geometryPass.frag");
	depthOnlyShader = new Util::Shader("assets/depthOnly.vert", "assets/depthOnly.frag");
	deferredLitShader = new Util::Shader("assets/postprocess.vert", "assets/deferredLit.frag");
	postprocessShader = new Util::Shader("assets/postprocess.vert", "assets/postprocess.frag");

	//Model setup
	//TODO: Do some crazy duplication here
	monkeyModel = new Util::Model("assets/Suzanne.obj");
	//Using a basic plane mesh from Maya since procGen doesn't calculate TBN
	planeModel = new Util::Model("assets/plane.fbx");
	sphereModel = new Util::Model("assets/sphere.fbx");

	//Load textures
	brickColorTexture = ew::loadTexture("assets/brick2_color.jpg");
	brickNormalTexture = ew::loadTexture("assets/brick2_normal.jpg");

	rockColorTexture = ew::loadTexture("assets/rock_color.jpg");
	rockNormalTexture = ew::loadTexture("assets/rock_normal.jpg");

	currentColorTexture = brickColorTexture;
	currentNormalTexture = brickNormalTexture;
}

void cleanup()
{
	delete gBufferShader;
	delete depthOnlyShader;
	delete deferredLitShader;
	delete postprocessShader;

	delete monkeyModel;
	delete planeModel;
	delete sphereModel;
}

const auto SHADING_MODEL_TO_COLOR = std::map<ShadingModel, glm::vec3>
{
	{ ShadingModel::LIT, glm::vec3(1.f, 0.f, 0.f) },
	{ ShadingModel::UNLIT, glm::vec3(0.f, 1.f, 0.f) },
};

//From https://mokole.com/palette.html and https://www.rapidtables.com/convert/color/hex-to-rgb.html
constexpr glm::vec4 DEFAULT_MONKEY_LIGHT_COLORS[MAX_LIGHTS_PER_MONKEY] = 
{
	glm::vec4(0.1843137254901961, 0.30980392156862746, 0.30980392156862746, 1.f), //darkslategray
	glm::vec4(0.4196078431372549, 0.5568627450980392, 0.13725490196078433, 1.f), //olivedrab
	glm::vec4(0.5019607843137255, 0.f, 0.f, 1.f), //maroon
	glm::vec4(0.09803921568627451, 0.09803921568627451, 0.4392156862745098, 1.f), //midnightblue
	glm::vec4(1.f, 0.f, 0.f, 1.f), //red
	glm::vec4(0.f, 0.807843137254902, 0.8196078431372549, 1.f), //darkturquoise
	glm::vec4(1.f, 0.6470588235294118, 0.f, 1.f), //orange
	glm::vec4(1.f, 1.f, 0.f, 1.f), //yellow
	glm::vec4(0.f, 0.f, 0.803921568627451, 1.f), //mediumblue
	glm::vec4(0.f, 1.f, 0.f, 1.f), //lime
	glm::vec4(0.f, 0.9803921568627451, 0.6039215686274509, 1.f), //mediumspringgreen
	glm::vec4(1.f, 0.f, 1.f, 1.f), //fuchsia
	glm::vec4(0.11764705882352941, 0.5647058823529412, 1.f, 1.f), //dodgerblue
	glm::vec4(0.8666666666666667, 0.6274509803921569, 0.8666666666666667, 1.f), //plum
	glm::vec4(1.f, 0.0784313725490196, 0.5764705882352941, 1.f), //deeppink
	glm::vec4(0.9607843137254902, 0.8705882352941177, 0.7019607843137254, 1.f), //wheat
};
std::vector<glm::vec4> activeMonkeyLightColors(MAX_LIGHTS_PER_MONKEY);

void upateMonkeyLightColors()
{
	//Reset lights
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		pointLights[i].color = glm::vec4(0.f);
	}

	activeMonkeyLightColors.resize(lightsPerMonkey);

	if (lightsPerMonkey > prevLightsPerMonkey)
	{
		for (int i = prevLightsPerMonkey; i < lightsPerMonkey; i++)
		{
			activeMonkeyLightColors[i] = DEFAULT_MONKEY_LIGHT_COLORS[i];
		}
	}

	prevLightsPerMonkey = lightsPerMonkey;
}

void drawScene(Util::Shader* shader, const glm::mat4& viewMatrix)
{
	static float prevTime = -1.f;
	int currentLightIndex = 0;

	shader->use();
	shader->setMat4("_view", viewMatrix);

	ew::Transform planeT;
	static ew::Transform monkeyT;

	//Monkey go spinny
	if (prevTime != prevFrameTime)
	{
		prevTime = prevFrameTime;
		monkeyT.rotation = glm::rotate(monkeyT.rotation, deltaTime, glm::vec3(0.f, 1.f, 0.f));
	}

	for (int x = 0; x < sceneGridSize; x++)
	{
		for (int z = 0; z < sceneGridSize; z++)
		{
			float xCenter = float(x) - float(sceneGridSize) / 2.f;
			float zCenter = float(z) - float(sceneGridSize) / 2.f;
			
			planeT.position = glm::vec3(xCenter * 10.f, -2.f, zCenter * 10.f);
			monkeyT.position = glm::vec3(xCenter * 10.f, 0.f, zCenter * 10.f);
			
			if (shader == gBufferShader)
			{
				gBufferShader->setSubroutine(GL_FRAGMENT_SHADER, {
					std::make_pair("_albedoFunction", "albedoFromTexture"),
					std::make_pair("_normalFunction", "normalFromTexture")
				});
			}
			gBufferShader->setVec3("_shadingModelColor", SHADING_MODEL_TO_COLOR.at(ShadingModel::LIT));

			shader->setMat4("_model", planeT.modelMatrix());
			planeModel->draw();

			shader->setMat4("_model", monkeyT.modelMatrix());
			monkeyModel->draw();

			//Lights
			if (shader == gBufferShader)
			{
				gBufferShader->setSubroutine(GL_FRAGMENT_SHADER, {
					std::make_pair("_albedoFunction", "albedoFromColor"),
					std::make_pair("_normalFunction", "normalFromMesh")
				});
				gBufferShader->setVec3("_shadingModelColor", SHADING_MODEL_TO_COLOR.at(ShadingModel::UNLIT));
			}

			float angleStep = glm::radians(360.f / float(lightsPerMonkey));
			float lightRadius = 3.f;
			for (int l = 0; l < lightsPerMonkey; l++)
			{
				ew::Transform lightT;
				float localX = glm::cos(angleStep * l) * lightRadius;
				float localZ = glm::sin(angleStep * l) * lightRadius;

				lightT.position = glm::vec3(localX + xCenter * 10.f, 0.f, localZ + zCenter * 10.f);
				lightT.scale = glm::vec3(0.3f);

				if (shader == gBufferShader)
				{
					PointLight currentLight;
					currentLight.color = activeMonkeyLightColors[l];
					currentLight.position = lightT.position;
					//printf("[%i] Position: X=%f Y=%f Z=%f, Color: R=%f, G=%f, B=%f\n", currentLightIndex, pointLights[currentLightIndex].position.x, pointLights[currentLightIndex].position.y, pointLights[currentLightIndex].position.z, pointLights[currentLightIndex].color.r, pointLights[currentLightIndex].color.g, pointLights[currentLightIndex].color.b);
					pointLights[currentLightIndex++] = currentLight;
				}

				shader->setVec4("_solidColor", activeMonkeyLightColors[l]);
				shader->setMat4("_model", lightT.modelMatrix());
				sphereModel->draw();
			}
		}
	}
}

int main() {
	GLFWwindow* window = initWindow("Assignment 3", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
	createPostprocessFramebuffer(screenWidth, screenHeight);
	
	setupScene();
	upateMonkeyLightColors();

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

		//monkeyTransform.rotation = glm::rotate(monkeyTransform.rotation, deltaTime, glm::vec3(0.f, 1.f, 0.f));
		glBindTextureUnit(0, gBuffer.getDepthAttachment());
		glBindTextureUnit(1, currentColorTexture);
		glBindTextureUnit(2, currentNormalTexture);

		//GBuffer geometry pass
		glCullFace(GL_BACK);
		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.getFBO());
		glViewport(0, 0, gBuffer.getSize().x, gBuffer.getSize().y);
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		drawScene(gBufferShader, camera.projectionMatrix() * camera.viewMatrix());
		gBufferShader->setInt("_mainTex", 1);
		gBufferShader->setInt("_normalTex", 2);
		
		//Shadow map pass
		glCullFace(GL_FRONT);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer.getFBO());
		glViewport(0, 0, shadowFramebuffer.getSize().x, shadowFramebuffer.getSize().y);
		glClear(GL_DEPTH_BUFFER_BIT);

		glm::mat4 lightView = glm::lookAt(directionalLight.position, glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f));
		glm::mat4 lightMatrix = directionalLight.projectionMatrix() * lightView;
		drawScene(depthOnlyShader, lightMatrix);

		//Deferred lit pass
		glCullFace(GL_BACK);
		glViewport(0, 0, gBuffer.getSize().x, gBuffer.getSize().y);
		//TODO: Draw to post process buffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindTextureUnit(0, gBuffer.getColorAttachment(0));
		glBindTextureUnit(1, gBuffer.getColorAttachment(1));
		glBindTextureUnit(2, gBuffer.getColorAttachment(2));
		glBindTextureUnit(3, gBuffer.getColorAttachment(3));
		glBindTextureUnit(4, shadowFramebuffer.getDepthAttachment());

		glNamedBufferSubData(lightsUBO, 0, sizeof(pointLights), pointLights);
		
		deferredLitShader->use();
		deferredLitShader->setMat4("_lightViewProjection", lightMatrix);
		deferredLitShader->setVec3("_litShadingModelColor", SHADING_MODEL_TO_COLOR.at(ShadingModel::LIT));
		deferredLitShader->setVec3("_unlitShadingModelColor", SHADING_MODEL_TO_COLOR.at(ShadingModel::UNLIT));
		deferredLitShader->setVec3("_cameraPosition", camera.position);
		deferredLitShader->setVec3("_lightPosition", directionalLight.position);
		deferredLitShader->setFloat("_shadowBrightness", shadowBrightness);
		deferredLitShader->setFloat("_shadowMinBias", shadowMinBias);
		deferredLitShader->setFloat("_shadowMaxBias", shadowMaxBias);
		deferredLitShader->setVec3("_ambientColor", ambientColor);
		deferredLitShader->setVec3("_lightColor", lightColor);
		deferredLitShader->setFloat("_material.ambientStrength", material.ambientStrength);
		deferredLitShader->setFloat("_material.diffuseStrength", material.diffuseStrength);
		deferredLitShader->setFloat("_material.specularStrength", material.specularStrength);
		deferredLitShader->setFloat("_material.shininess", material.shininess);

		glBindVertexArray(screenVAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		//Post process pass
		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//glBindTextureUnit(0, postprocessFramebuffer.getColorAttachment());
		//glBindTextureUnit(1, postprocessFramebuffer.getDepthAttachment());
		//postprocessShader->use();

		//postprocessShader->setSubroutine(GL_FRAGMENT_SHADER, {
		//	std::make_pair("_blurFunction", boxBlurEnable ? "blurEnabled" : "blurDisabled"),
		//	std::make_pair("_dofFunction", dofEnable ? "dofEnabled" : "dofDisabled"),
		//	std::make_pair("_chromaticAberrationFunction", chromaticAberrationEnable ? "chromaticAberrationEnabled" : "chromaticAberrationDisabled") });

		//postprocessShader->setInt("_colorBuffer", 0);
		//postprocessShader->setInt("_depthBuffer", 1);
		//postprocessShader->setVec2("_focusPoint", focusPoint);
		//postprocessShader->setInt("_boxBlurSize", boxBlurSize);
		//postprocessShader->setFloat("_chromaticAberrationSize", chromaticAberrationSize);
		//postprocessShader->setFloat("_dofMinDistance", dofMinDistance);
		//postprocessShader->setFloat("_dofMaxDistance", dofMaxDistance);
		//postprocessShader->setInt("_dofBlurSize", dofBlurSize);

		//glBindVertexArray(screenVAO);
		//glDrawArrays(GL_TRIANGLES, 0, 3);

		drawUI();

		glfwSwapBuffers(window);
	}

	cleanup();

	printf("Shutting down...");
}

void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	if (ImGui::CollapsingHeader("Scene"))
	{
		ImGui::SliderInt("Grid size", &sceneGridSize, 1, MAX_GRID_SIZE);
		ImGui::SliderInt("Lights per monkey", &lightsPerMonkey, 1, MAX_LIGHTS_PER_MONKEY);
		if (lightsPerMonkey != prevLightsPerMonkey) upateMonkeyLightColors();
		ImGui::Indent();
		for (int i = 0; i < lightsPerMonkey; i++)
		{
			ImGui::PushID(i);
			ImGui::ColorEdit4("Color", &activeMonkeyLightColors.data()[i].r);
			ImGui::PopID();
		}
		ImGui::Unindent();
	}
	if (ImGui::CollapsingHeader("Global light"))
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

	ImGui::Begin("GBuffers");
	ImVec2 texSize = ImVec2(gBuffer.getSize().x / 8.f, gBuffer.getSize().y / 8.f);
	for (size_t i = 0; i < gBuffer.getNumColorAttachments(); i++)
	{
		ImGui::Image(reinterpret_cast<ImTextureID>(gBuffer.getColorAttachment(i)), texSize, ImVec2(0, 1), ImVec2(1, 0));
	}
	ImGui::Image(reinterpret_cast<ImTextureID>(gBuffer.getDepthAttachment()), texSize, ImVec2(0, 1), ImVec2(1, 0));
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

