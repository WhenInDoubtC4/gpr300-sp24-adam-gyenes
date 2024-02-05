#include "framebuffer.h"

constexpr int MAX_COLOR_ATTACHMENTS = 8;

//Why :(
constexpr GLenum COLOR_ATTACHMENTS[MAX_COLOR_ATTACHMENTS] = {
	GL_COLOR_ATTACHMENT0,
	GL_COLOR_ATTACHMENT1,
	GL_COLOR_ATTACHMENT2,
	GL_COLOR_ATTACHMENT3,
	GL_COLOR_ATTACHMENT4,
	GL_COLOR_ATTACHMENT5,
	GL_COLOR_ATTACHMENT6,
	GL_COLOR_ATTACHMENT7,
};

namespace Util
{
	Framebuffer::Framebuffer(const glm::vec2& size)
		: _size(size)
	{
		//Delete previous attachments
		_colorAttachments.clear();

		glCreateFramebuffers(1, &_fbo);
	}

	GLuint Framebuffer::addColorAttachment(GLenum colorFormat)
	{
		if (_colorAttachments.size() >= MAX_COLOR_ATTACHMENTS)
		{
			printf("Framebuffer%i exceeds maximum number of color attachments\n", _fbo);
			return -1;
		}

		GLuint colorAttachment;

		glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

		glGenTextures(1, &colorAttachment);
		glBindTexture(GL_TEXTURE_2D, colorAttachment);
		glTexStorage2D(GL_TEXTURE_2D, 1, colorFormat, _size.x, _size.y);
		glFramebufferTexture(GL_FRAMEBUFFER, COLOR_ATTACHMENTS[_colorAttachments.size()], colorAttachment, 0);

		_colorAttachments.push_back(colorAttachment);
		return colorAttachment;
	}

	GLuint Framebuffer::addDepthAttachment()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

		glGenTextures(1, &_depthAttachment);
		glBindTexture(GL_TEXTURE_2D, _depthAttachment);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT16, _size.x, _size.y);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthAttachment, 0);

		return _depthAttachment;
	}

	bool Framebuffer::isComplete() const
	{
		glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
		return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}

	GLuint Framebuffer::getColorAttachment(int index) const
	{
		return _colorAttachments[index];
	}
}