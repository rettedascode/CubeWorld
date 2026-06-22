#pragma once

#include "ce/render/BufferLayout.h"

#include <cstdint>

namespace ce {

// RAII wrapper over an OpenGL vertex buffer object (VBO).
class VertexBuffer {
public:
    VertexBuffer(const void* data, uint32_t size); // static (data uploaded now)
    explicit VertexBuffer(uint32_t size);           // dynamic (filled later)
    ~VertexBuffer();

    VertexBuffer(const VertexBuffer&)            = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    void bind() const;
    void unbind() const;
    void setData(const void* data, uint32_t size);

    const BufferLayout& layout() const { return m_layout; }
    void setLayout(const BufferLayout& layout) { m_layout = layout; }

private:
    uint32_t     m_id = 0;
    BufferLayout m_layout;
};

// RAII wrapper over an OpenGL element/index buffer object (EBO). 32-bit indices.
class IndexBuffer {
public:
    IndexBuffer(const uint32_t* indices, uint32_t count);
    ~IndexBuffer();

    IndexBuffer(const IndexBuffer&)            = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;

    void bind() const;
    void unbind() const;

    uint32_t count() const { return m_count; }

private:
    uint32_t m_id    = 0;
    uint32_t m_count = 0;
};

} // namespace ce
