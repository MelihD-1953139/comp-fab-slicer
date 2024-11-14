#pragma once

class Framebuffer {
   public:
	Framebuffer(int width, int height);
	~Framebuffer();

	void bind();
	void unbind();
	void clear();

	void resize(int width, int height);

	unsigned int getTexture() const { return m_texture; }

   private:
	unsigned int m_fbo;
	unsigned int m_texture;
	unsigned int m_rbo;
};