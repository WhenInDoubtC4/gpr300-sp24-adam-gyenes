#version 450

struct Material
{
	float ambientStrength;
	float diffuseStrength;
	float specularStrength;
	float shininess;
};

in vec2 UV;

uniform layout(location = 0) sampler2D _gPosition;
uniform layout(location = 1) sampler2D _gNormal;
uniform layout(location = 2) sampler2D _gAlbedo;
uniform layout(location = 3) sampler2D _gShadowMap;

uniform mat4 _lightViewProjection;

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

void main()
{
	vec3 lightSpacePos = vec3(_lightViewProjection * texture(_gPosition, UV));

	vec3 normal = texture(_gNormal, UV).rgb;
	normal = normalize(normal * 2.0 - 1.0);
	vec3 position = texture(_gPosition, UV).xyz;
	vec3 _lightDirection = normalize(position - _lightPosition);

	vec3 albedo = texture(_gAlbedo, UV).rgb;
}