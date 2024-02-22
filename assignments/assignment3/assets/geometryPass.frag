#version 450

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gAlbedo;

in Surface
{
	vec3 pos;
	mat3 tbn;
	vec2 UV;
} fs_in;

uniform sampler2D _mainTex;
uniform sampler2D _normalTex;

void main()
{
	gPosition = fs_in.pos;
	gAlbedo = texture(_mainTex, fs_in.UV).rgb;
	//gNormal = transpose(fs_in.tbn)[2]; //WS normal form TBN
	gNormal = fs_in.tbn * texture(_normalTex, fs_in.UV).rgb; //WS normal
}