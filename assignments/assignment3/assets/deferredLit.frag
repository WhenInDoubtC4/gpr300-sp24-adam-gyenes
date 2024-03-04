#version 450

#define SHADING_MODEL_COLOR_MATCH_TRESHOLD 0.1
//MUST match values in mainGlobals.h
#define MAX_GRID_SIZE 16
#define MAX_LIGHTS_PER_MONKEY 16
#define MAX_POINT_LIGHTS MAX_GRID_SIZE * MAX_LIGHTS_PER_MONKEY

struct PointLight
{
	vec3 position;
	float radius;
	vec4 color;
};

struct Material
{
	float ambientStrength;
	float diffuseStrength;
	float specularStrength;
	float shininess;
};

in vec2 UV;

uniform layout(binding = 0) sampler2D _gPosition;
uniform layout(binding = 1) sampler2D _gNormal;
uniform layout(binding = 2) sampler2D _gAlbedo;
uniform layout(binding = 3) sampler2D _gShadingModel;
uniform layout(binding = 4) sampler2D _gShadowMap;

uniform mat4 _lightViewProjection;

uniform vec3 _litShadingModelColor;
uniform vec3 _unlitShadingModelColor;

uniform PointLight _pointLights[MAX_POINT_LIGHTS];

uniform vec3 _cameraPosition;
uniform vec3 _lightPosition;
uniform vec3 _ambientColor;
uniform vec3 _lightColor;
uniform float _shadowBrightness;
uniform float _shadowMinBias;
uniform float _shadowMaxBias;
uniform Material _material;

out vec4 FragColor;

float calcShadow(sampler2D shadowMap, vec3 normal, vec3 toLight, vec4 lightSpacePosition)
{
	vec3 samplerCoord = lightSpacePosition.xyz / lightSpacePosition.w;
	samplerCoord = samplerCoord * 0.5 + 0.5;

	float bias = max(_shadowMaxBias * (1.0 - dot(normal, toLight)), _shadowMinBias);
	float depth = samplerCoord.z - bias;
	float shadowMapDepth = texture(shadowMap, samplerCoord.xy).b;

	//PCF filtering
	float totalShadow = 0.0;
	vec2 texelOffset = 1.0 / textureSize(_gShadowMap, 0);
	for (int y = -1; y <= 1; y++)
	{
		for (int x = -1; x <= 1; x++)
		{
			vec2 uv = samplerCoord.xy + vec2(x * texelOffset.x, y * texelOffset.y);
			totalShadow += step(texture(_gShadowMap, uv).r, depth);
		}
	}

	return totalShadow / 9.0;
}

float attenuateLinear(float dist, float radius)
{
	return clamp((radius - dist) / radius, 0.0, 1.0);
}

float attenuateExponential(float dist, float radius)
{
	float i = clamp(1.0 - pow(dist / radius, 4.0), 0.0, 1.0);
	return i * i;
}

vec3 calcPointLight(PointLight light, vec3 worldPosition, vec3 normal, vec3 toCamera)
{
	vec3 diff = light.position - worldPosition;
	vec3 toLight = normalize(diff);
	
	float diffuseFactor = max(dot(normal, toLight), 0.0);
	vec3 h = normalize(toLight + toCamera);
	float specularFactor = pow(max(dot(normal, h), 0.0), _material.shininess);

	vec3 lightColor = light.color.rgb * diffuseFactor * _material.diffuseStrength;
	lightColor += light.color.rgb * specularFactor * _material.specularStrength;

	//Attenuation
	float dist = length(diff);
	//TODO: Might want to add subroutines
	lightColor *= attenuateLinear(dist, light.radius);

	return lightColor;
}

void main()
{
	vec3 albedo = texture(_gAlbedo, UV).rgb;

	//Unlit shader
	if (abs(length(_unlitShadingModelColor - texture(_gShadingModel, UV).rgb)) < SHADING_MODEL_COLOR_MATCH_TRESHOLD)
	{
		FragColor = vec4(albedo, 1.0);
		return;
	}

	vec4 lightSpacePos = _lightViewProjection * texture(_gPosition, UV);

	vec3 normal = texture(_gNormal, UV).rgb;
	vec3 position = texture(_gPosition, UV).xyz;
	vec3 _lightDirection = normalize(/*position*/ - _lightPosition);
	vec3 toLight = normal * -_lightDirection;
	vec3 toCamera = normalize(_cameraPosition - position);
	float diffuseFactor = max(dot(normal, toLight), 0.0);

	//Specular reflection
	vec3 h = normalize(toLight + toCamera);
	float specularFactor = pow(max(dot(normal, h), 0.0), _material.shininess);

	float shadow = min(calcShadow(_gShadowMap, normal, toLight, lightSpacePos), 1.0 - _shadowBrightness);

	vec3 light = _ambientColor * _material.ambientStrength;
	light += _lightColor * diffuseFactor * _material.diffuseStrength;
	light += _lightColor * specularFactor * _material.specularStrength;

	//Point lights
	for (int i = 0; i < MAX_POINT_LIGHTS; i++)
	{
		light += calcPointLight(_pointLights[i], position, normal, toCamera);
	}

	light *= 1.0 - shadow;

	FragColor = vec4(albedo * light, 1.0);
}