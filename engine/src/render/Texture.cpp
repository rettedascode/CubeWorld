#include "ce/render/Texture.h"
#include "ce/util/Log.h"

#include <glad/glad.h>

// Compile the stb_image implementation into this translation unit only.
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ce {

Texture2D::Texture2D(const std::string& path) {
    stbi_set_flip_vertically_on_load(1); // OpenGL's texture origin is bottom-left
    int w = 0, h = 0, channels = 0;
    stbi_uc* data = stbi_load(path.c_str(), &w, &h, &channels, 0);
    if (!data) {
        CE_LOG_ERROR("Failed to load texture '", path, "': ", stbi_failure_reason());
        return;
    }
    create(data, static_cast<uint32_t>(w), static_cast<uint32_t>(h), channels);
    stbi_image_free(data);
    CE_LOG_INFO("Loaded texture '", path, "' (", w, "x", h, ", ", channels, " channels)");
}

Texture2D::Texture2D(uint32_t width, uint32_t height, const void* rgba) {
    create(rgba, width, height, 4);
}

void Texture2D::create(const void* data, uint32_t width, uint32_t height, int channels) {
    m_width  = width;
    m_height = height;

    GLenum internalFormat = (channels == 4) ? GL_RGBA8 : GL_RGB8;
    GLenum dataFormat     = (channels == 4) ? GL_RGBA  : GL_RGB;

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Tightly-packed rows; default unpack alignment of 4 would corrupt RGB images
    // whose row length isn't a multiple of 4.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(internalFormat),
                 static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
                 dataFormat, GL_UNSIGNED_BYTE, data);
}

Texture2D::~Texture2D() { glDeleteTextures(1, &m_id); }

void Texture2D::bind(uint32_t slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, m_id);
}

} // namespace ce
