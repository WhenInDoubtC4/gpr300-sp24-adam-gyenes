#pragma once

//Structs
struct Material
{
	float ambientStrength = 0.6f;
	float diffuseStrength = 1.f;
	float specularStrength = 1.f;
	float shininess = 128;
};

struct PointLight
{
	glm::vec3 position;
	float radius;
	glm::vec4 color;
};

//Global state
constexpr glm::vec3 CAMERA_INIT_POSITION = glm::vec3(0.f, 0.f, 5.f);
constexpr glm::vec3 CAMERA_INIT_TARGET = glm::vec3(0.f);

int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

//Camera
ew::Camera camera;
ew::CameraController cameraController;
float cameraFov = 60.f;

//Lighting and texturing
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

Util::Framebuffer gBuffer;

constexpr int MAX_POINT_LIGHTS = 1024;
PointLight pointLights[MAX_POINT_LIGHTS];

//Shaders and models
//Forced to use pointers here since it has no default constructor
Util::Shader* gBufferShader;
Util::Shader* depthOnlyShader;
Util::Shader* deferredLitShader;
Util::Shader* postprocessShader;

Util::Model* monkeyModel;
Util::Model* planeModel;
Util::Model* sphereModel;

int sceneGridSize = 8;
constexpr int MAX_LIGHTS_PER_MONKEY = 16;
int lightsPerMonkey = 4;
int prevLightsPerMonkey = 0;