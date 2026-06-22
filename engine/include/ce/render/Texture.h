#pragma once

#include <cstdint>
#include <string>

namespace ce {

// RAII wrapper over an OpenGL 2D texture. Uses NEAREST filtering, which suits
// pixel-art / voxel atlases. Can be built from an image file (via stb_image) or
// from raw RGBA pixels.
class Texture2D {
public:
    explicit Texture2D(const std::string& path);             // load from file
    Texture2D(uint32_t width, uint32_t height, const void* rgba); // from raw RGBA8
    ~Texture2D();

    Texture2D(const Texture2D&)            = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    void bind(uint32_t slot = 0) const;

    uint32_t width()  const { return m_width; }
    uint32_t height() const { return m_height; }
    bool     isLoaded() const { return m_id != 0; }

private:
    void create(const void* data, uint32_t width, uint32_t height, int channels);

    uint32_t m_id     = 0;
    uint32_t m_width  = 0;
    uint32_t m_height = 0;
};

} // namespace ce
