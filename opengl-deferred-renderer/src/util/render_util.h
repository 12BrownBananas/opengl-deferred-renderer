#ifndef RENDER_UTIL_H
#define RENDER_UTIL_H

#include <unordered_map>
#include <glad/glad.h>
#include <memory>

namespace Framebuffer {
	//contains a pointer to a texture and some useful side-information about that texture that isn't so easy to query OpenGL for.
	class TextureMarker {
	public:
		TextureMarker(float width, float height, unsigned int type = GL_TEXTURE_2D, unsigned int internalFormat = GL_RGBA, unsigned int format = GL_RGBA, unsigned int* data = NULL);
		float getWidth() const;
		float getHeight() const;
		unsigned int getTexture() const;
		void bindTexture() const;
		void bindToBoundFramebuffer(unsigned int attachmentSlot) const;
		~TextureMarker();
	private:
		unsigned int texture, type;
		float width, height;
	};

	//Higher-level abstraction class to condense definition of framebuffers and organize their various components
	class Framebuffer {
	public:
		Framebuffer(float width, float height);
		void generateTexture(unsigned int attachmentSlot = GL_COLOR_ATTACHMENT0, unsigned int internalColorFormat = GL_RGBA, unsigned int colorFormat = GL_RGBA, unsigned int* data = NULL, unsigned int minFilter = GL_NEAREST, unsigned int magFilter = GL_NEAREST, unsigned int wrapS = GL_CLAMP_TO_EDGE, unsigned int wrapT = GL_CLAMP_TO_EDGE);
		void bindTexture(unsigned int attachmentSlot = GL_COLOR_ATTACHMENT0);
		void bind(unsigned int bindingSlot = GL_FRAMEBUFFER);
		virtual void init(bool includeRenderbuffer = false, unsigned int renderBufferStorage = GL_DEPTH_COMPONENT, unsigned int renderBufferAttachment = GL_DEPTH_ATTACHMENT);
		unsigned int getTexture(unsigned int attachmentSlot = GL_COLOR_ATTACHMENT0);
		void setDrawBuffers() const;
		~Framebuffer();
	protected:
		bool initialized = false;
		float width, height;
		unsigned int framebuffer, renderbuffer;
		std::unordered_map<unsigned int, std::unique_ptr<TextureMarker>> texturesAndAttachments;
		
	};

	class MultisampleFramebuffer : Framebuffer {
	public:
		MultisampleFramebuffer(float width, float height, int samples, unsigned int internalRenderBufferFormat = GL_DEPTH24_STENCIL8);
		void init(bool includeRenderbuffer = false, unsigned int renderBufferStorage = GL_DEPTH_COMPONENT, unsigned int renderBufferAttachment = GL_DEPTH_STENCIL_ATTACHMENT);
	protected:
		int samples;
		unsigned int internalRBOFormat;
	};

	void reset_framebuffer_bindings();
	bool check_if_bound_framebuffer_is_complete();
}

#endif