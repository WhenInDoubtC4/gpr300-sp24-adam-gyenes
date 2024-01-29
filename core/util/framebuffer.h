#pragma once

#include "../ew/external/glad.h"

namespace Util
{
	struct Framebuffer
	{
		GLuint fbo;
		GLuint colorBuffer;
		GLuint depthBuffer;
		unsigned int width;
		unsigned int height;

		bool isComplete() const;
	};

	Framebuffer createFramebuffer(const unsigned int width, const unsigned int height, GLenum colorFormat = GL_RGBA8);
}