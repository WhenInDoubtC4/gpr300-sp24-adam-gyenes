#version 450

out vec4 FragColor;

in vec2 UV;

uniform sampler2D _colorBuffer;

void main()
{
	vec3 color = 1.0 - texture(_colorBuffer, UV).rgb;
	FragColor = vec4(color, 1.0);
}