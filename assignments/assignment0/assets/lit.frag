#version 450

in Surface
{
	vec3 pos;
	vec3 normal;
	vec2 UV;
} fs_in;

struct Material
{
	float ambientStrength;
	float diffuseStrength;
	float specularStrength;
	float shininess;
};

uniform vec3 _cameraPosition;
uniform vec3 _lightDirection = vec3(0.0, -1.0, 0.0);
uniform vec3 _ambientColor;
uniform vec3 _lightColor;
uniform sampler2D _mainTex;
uniform Material _material;

out vec4 FragColor;

void main()
{
	//Light calculations
	vec3 normal = normalize(fs_in.normal);
	vec3 toLight = -_lightDirection;
	float diffuseFactor = max(dot(normal, toLight), 0.0);
	
	//Specular reflection
	vec3 toCamera = normalize(_cameraPosition - fs_in.pos);
	vec3 h = normalize(toLight + toCamera);
	float specularFactor = pow(max(dot(normal, h), 0.0), _material.shininess);

	vec3 light = _ambientColor * _material.ambientStrength;
	light += _lightColor * diffuseFactor * _material.diffuseStrength;
	light += _lightColor * specularFactor * _material.specularStrength;
	vec3 object = texture(_mainTex, fs_in.UV).rgb;

	FragColor = vec4(object * light, 1.0);
}