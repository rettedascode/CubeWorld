#include "ce/render/Buffer.h"

#include <glad/glad.h>

namespace ce {

// ---- VertexBuffer ----------------------------------------------------------

VertexBuffer::VertexBuffer(const void* data, uint32_t size) {
    glGenBuffers(1, &m_id);
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

VertexBuffer::VertexBuffer(uint32_t size) {
    glGenBuffers(1, &m_id);
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
}

VertexBuffer::~VertexBuffer() { glDeleteBuffers(1, &m_id); }

void VertexBuffer::bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_id); }
void VertexBuffer::unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

void VertexBuffer::setData(const void* data, uint32_t size) {
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

// ---- IndexBuffer -----------------------------------------------------------

IndexBuffer::IndexBuffer(const uint32_t* indices, uint32_t count) : m_count(count) {
    glGenBuffers(1, &m_id);
    // Bind as GL_ARRAY_BUFFER here to avoid disturbing whatever VAO's element
    // binding is current; the VertexArray rebinds it as the element buffer.
    glBindBuffer(GL_ARRAY_BUFFER, m_id);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
}

IndexBuffer::~IndexBuffer() { glDeleteBuffers(1, &m_id); }

void IndexBuffer::bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_id); }
void IndexBuffer::unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

} // namespace ce
