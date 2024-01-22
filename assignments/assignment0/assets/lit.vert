#version 450

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;

uniform mat4 _model;
uniform mat4 _viewProjection;

out Surface
{
	vec3 normal;
	vec2 UV;
} vs_out;

void main()
{
	vs_out.normal = vNormal;
	vs_out.UV = vUV;
	gl_Position = _viewProjection * _model * vec4(vPos, 1.0);
}