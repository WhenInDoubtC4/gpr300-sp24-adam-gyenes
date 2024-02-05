#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "../ew/external/glad.h"

namespace Util
{
	class Framebuffer
	{
	public:
		Framebuffer() {};
		Framebuffer(const glm::vec2& size);

		GLuint addColorAttachment(GLenum colorFormat = GL_RGBA8);
		GLuint addDepthAttachment();

		bool isComplete() const;
		glm::vec2 getSize() const { return _size; };
		GLuint getFBO() const { return _fbo; };
		GLuint getColorAttachment(int index = 0) const;
		GLuint getDepthAttachment() const { return _depthAttachment; };

	private:
		glm::vec2 _size;
		GLuint _fbo;
		std::vector<GLuint> _colorAttachments;
		GLuint _depthAttachment;
	};
}