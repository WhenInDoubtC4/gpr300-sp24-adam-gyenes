#include "framebuffer.h"

namespace Util
{
	Framebuffer createFramebuffer(const unsigned int width, const unsigned int height, GLenum colorFormat)
	{
		Framebuffer result;
		result.width = width;
		result.height = height;

		//Create FBO
		glCreateFramebuffers(1, &result.fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, result.fbo);

		//Create and attach color buffer texture
		glGenTextures(1, &result.colorBuffer);
		glBindTexture(GL_TEXTURE_2D, result.colorBuffer);
		glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, result.width, result.height);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, result.colorBuffer, 0);

		//Create and attach depth buffer texture
		glGenTextures(1, &result.depthBuffer);
		glBindTexture(GL_TEXTURE_2D, result.depthBuffer);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, result.width, result.height);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, result.depthBuffer, 0);

		return result;
	}

	bool Framebuffer::isComplete() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}
}