#version 450

in Surface
{
	vec3 normal;
	vec2 UV;
} fs_in;

uniform sampler2D _mainTex;

out vec4 FragColor;

void main()
{
	vec4 color = texture(_mainTex, fs_in.UV);
	FragColor = color;
}