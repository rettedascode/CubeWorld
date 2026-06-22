#pragma once

#include "ce/render/Buffer.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace ce {

// RAII wrapper over an OpenGL vertex array object (VAO). It owns the vertex and
// index buffers added to it and configures attribute pointers from each vertex
// buffer's BufferLayout.
class VertexArray {
public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&)            = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    void bind() const;
    void unbind() const;

    // The vertex buffer MUST have a layout set before being added.
    void addVertexBuffer(std::unique_ptr<VertexBuffer> buffer);
    void setIndexBuffer(std::unique_ptr<IndexBuffer> buffer);

    const IndexBuffer* indexBuffer() const { return m_indexBuffer.get(); }

private:
    uint32_t m_id          = 0;
    uint32_t m_attribIndex = 0;
    std::vector<std::unique_ptr<VertexBuffer>> m_vertexBuffers;
    std::unique_ptr<IndexBuffer>               m_indexBuffer;
};

} // namespace ce
