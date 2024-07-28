#include <unordered_map>
#include <memory>
#include <iostream>

#include <glad/glad.h>
#include <render_util.h>

namespace Framebuffer {
	/* TextureMarker */
	TextureMarker::TextureMarker(float width, float height, unsigned int type, unsigned int internalFormat, unsigned int format, unsigned int *data) {
		this->width = width;
		this->height = height;
		unsigned int t;
		glGenTextures(1, &t);
		glBindTexture(type, t);
		glTexImage2D(type, 0, internalFormat, width, height, 0, format, GL_FLOAT, data);
		this->texture = t;
		this->type = type;
	}
	float TextureMarker::getWidth() const {
		return this->width;
	}
	float TextureMarker::getHeight() const {
		return this->height;
	}
	unsigned int TextureMarker::getTexture() const {
		return this->texture;
	}
	void TextureMarker::bindTexture() const {
		glBindTexture(type, texture);
	}
	void TextureMarker::bindToBoundFramebuffer(unsigned int attachmentSlot) const {
		if (this->type == GL_TEXTURE_2D)
			glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentSlot, type, texture, 0);
	}
	TextureMarker::~TextureMarker() {
		glDeleteTextures(1, &(this->texture));
	}

	/* Framebuffer */
	Framebuffer::Framebuffer(float width, float height) {
		this->width = width;
		this->height = height;
		this->framebuffer = NULL;
		this->renderbuffer = NULL;
	}
	void Framebuffer::init(bool includeRenderbuffer, unsigned int renderBufferStorage, unsigned int renderBufferAttachment) {
		if (initialized) return;
		
		unsigned int fb;
		glGenFramebuffers(1, &fb);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);

		this->framebuffer = fb;

		if (includeRenderbuffer) {
			unsigned int rb;
			glGenRenderbuffers(1, &rb);
			glBindRenderbuffer(GL_RENDERBUFFER, rb);
			glRenderbufferStorage(GL_RENDERBUFFER, renderBufferStorage, width, height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, renderBufferAttachment, GL_RENDERBUFFER, rb);
			this->renderbuffer = rb;
		}
		initialized = true;
	}

	void Framebuffer::generateTexture(unsigned int attachmentSlot, unsigned int internalColorFormat, unsigned int colorFormat, unsigned int* data, unsigned int minFilter, unsigned int magFilter, unsigned int wrapS, unsigned int wrapT) {
		if (!initialized) return;
		glBindFramebuffer(GL_FRAMEBUFFER, this->framebuffer);
		std::unique_ptr<TextureMarker> texPtr(new TextureMarker(this->width, this->height, GL_TEXTURE_2D, internalColorFormat, colorFormat, NULL));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
		auto t = texPtr.get();
		t->bindToBoundFramebuffer(attachmentSlot);

		this->texturesAndAttachments.insert_or_assign(attachmentSlot, std::move(texPtr));
	}

	void Framebuffer::bindTexture(unsigned int attachmentSlot) {
		if (!initialized) return;
		auto search = texturesAndAttachments.find(attachmentSlot);
		if (search == texturesAndAttachments.end()) {
			std::cout << "ERROR (Framebuffer::bindTexture): Trying to bind unassigned attachment slot." << std::endl;
		}
		else {
			auto tex = search->second.get();
			tex->bindTexture();
		}
	}
	unsigned int Framebuffer::getTexture(unsigned int attachmentSlot) {
		if (!initialized) return 0;
		auto search = texturesAndAttachments.find(attachmentSlot);
		if (search == texturesAndAttachments.end()) {
			std::cout << "ERROR (Framebuffer::getTexture): Trying to get from unassigned attachment slot." << std::endl;
		}
		else {
			auto tex = search->second.get();
			return tex->getTexture();
		}
		return 0;
	}

	void Framebuffer::bind(unsigned int bindingSlot) {
		if (!initialized) return;
		glBindFramebuffer(bindingSlot, this->framebuffer);
	}

	void Framebuffer::setDrawBuffers() const {
		std::vector<unsigned int> attributes;
		for (auto i = texturesAndAttachments.begin(); i != texturesAndAttachments.end(); i++) {
			attributes.push_back(i->first);
		}
		glDrawBuffers(attributes.size(), attributes.data());
	}

	Framebuffer::~Framebuffer() {
		if (!initialized) return;
		glDeleteFramebuffers(1, &(this->framebuffer));
		if (this->renderbuffer != NULL) 
			glDeleteRenderbuffers(1, &(this->renderbuffer));
	}

	/* MultisampleFramebuffer */
	MultisampleFramebuffer::MultisampleFramebuffer(float width, float height, int samples, unsigned int internalRenderBufferFormat)
	: Framebuffer(width, height) {
		this->samples = samples;
		this->internalRBOFormat = internalRenderBufferFormat;
	}
	void MultisampleFramebuffer::init(bool includeRenderbuffer, unsigned int renderBufferStorage, unsigned int renderBufferAttachment) {
		if (initialized) return;
		//same as the base init, but configured for multisampling instead
		unsigned int fb;
		glGenFramebuffers(1, &fb);
		glBindFramebuffer(GL_FRAMEBUFFER, fb);

		this->framebuffer = fb;

		if (includeRenderbuffer) {
			unsigned int rb;
			glGenRenderbuffers(1, &rb);
			glBindRenderbuffer(GL_RENDERBUFFER, rb);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, this->samples, this->internalRBOFormat, this->width, this->height);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, renderBufferStorage, GL_RENDERBUFFER, rb);
			this->renderbuffer = rb;
		}
		else {
			this->renderbuffer = NULL;
		}
		initialized = true;
	}

	void reset_framebuffer_bindings() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	bool check_if_bound_framebuffer_is_complete() {
		return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}
}