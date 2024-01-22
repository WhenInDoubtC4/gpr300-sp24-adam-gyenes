#version 450

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;

uniform mat4 _model;
uniform mat4 _viewProjection;

out vec3 vs_normal;

void main()
{
	vs_normal = vNormal;
	gl_Position = _viewProjection * _model * vec4(vPos, 1.0);
}