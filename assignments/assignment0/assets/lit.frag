#version 450

in vec3 vs_normal;

out vec4 FragColor;

void main()
{
	FragColor = vec4(vs_normal * 0.5 + 0.5, 1.0);
}