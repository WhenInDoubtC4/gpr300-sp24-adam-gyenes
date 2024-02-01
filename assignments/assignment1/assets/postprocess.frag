#version 450

out vec4 FragColor;

in vec2 UV;

uniform sampler2D _colorBuffer;

uniform vec2 _focusPoint;

uniform int _boxBlurEnable;
uniform int _boxBlurSize;

uniform int _chromaticAberrationEnable;
uniform float _chromaticAberrationSize;

//From lecture slide and https://lettier.github.io/3d-game-shaders-for-beginners/blur.html
vec3 boxBlur(int size, sampler2D colorTexture, vec2 texelSize, vec2 UVs)
{
	vec3 color = vec3(0.0);

	for (int x = -size; x <= size; x++)
	{
		for (int y = -size; y <= size; y++)
		{
			vec2 offset = vec2(x, y) * texelSize;
			color += texture(colorTexture, UVs + offset).rgb;
		}
	}

	color /= pow(size * 2 + 1, 2);

	return color;
}

void main()
{
	vec3 color = texture(_colorBuffer, UV).rgb;
	vec2 texelSize = 1.0 / textureSize(_colorBuffer, 0).xy;

	//Box blur
	if (_boxBlurEnable == 1)
	{
		color = boxBlur(_boxBlurSize, _colorBuffer, texelSize, UV);
	}

	//Chromatic aberration
	//From https://www.shadertoy.com/view/wsdBWM
	//And https://lettier.github.io/3d-game-shaders-for-beginners/chromatic-aberration.html
	if (_chromaticAberrationEnable == 1)
	{
		vec2 direction = UV - _focusPoint;

		vec2 rUV = UV + (direction * vec2(_chromaticAberrationSize));
		vec2 gUV = UV + (direction * vec2(0.5 * _chromaticAberrationSize));
		vec2 bUV = UV + (direction * vec2(0.25 * _chromaticAberrationSize));

		if (_boxBlurEnable == 1)
		{
			color.r = boxBlur(_boxBlurSize, _colorBuffer, texelSize, rUV).r;
			color.g = boxBlur(_boxBlurSize, _colorBuffer, texelSize, gUV).g;
			color.b = boxBlur(_boxBlurSize, _colorBuffer, texelSize, bUV).b;
		}
		else
		{
			color.r = texture(_colorBuffer, rUV).r;
			color.g = texture(_colorBuffer, gUV).g;
			color.b = texture(_colorBuffer, bUV).b;
		}
	}
	
	FragColor = vec4(color, 1.0);
}