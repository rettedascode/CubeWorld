#include "ce/render/VertexArray.h"
#include "ce/util/Log.h"

#include <glad/glad.h>

namespace ce {

// Maps an engine ShaderDataType to the OpenGL scalar base type.
static GLenum glBaseType(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:
        case ShaderDataType::Float2:
        case ShaderDataType::Float3:
        case ShaderDataType::Float4: return GL_FLOAT;
        case ShaderDataType::Int:
        case ShaderDataType::Int2:
        case ShaderDataType::Int3:
        case ShaderDataType::Int4:   return GL_INT;
        case ShaderDataType::Bool:   return GL_BOOL;
        default:                     return 0;
    }
}

static bool isIntegerType(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Int:
        case ShaderDataType::Int2:
        case ShaderDataType::Int3:
        case ShaderDataType::Int4: return true;
        default:                   return false;
    }
}

VertexArray::VertexArray() { glGenVertexArrays(1, &m_id); }
VertexArray::~VertexArray() { glDeleteVertexArrays(1, &m_id); }

void VertexArray::bind() const { glBindVertexArray(m_id); }
void VertexArray::unbind() const { glBindVertexArray(0); }

void VertexArray::addVertexBuffer(std::unique_ptr<VertexBuffer> buffer) {
    if (buffer->layout().elements().empty()) {
        CE_LOG_ERROR("VertexBuffer added with an empty layout");
        return;
    }

    glBindVertexArray(m_id);
    buffer->bind();

    const auto& layout = buffer->layout();
    for (const auto& element : layout) {
        glEnableVertexAttribArray(m_attribIndex);
        const void* offset = reinterpret_cast<const void*>(static_cast<uintptr_t>(element.offset));

        if (isIntegerType(element.type)) {
            // Integer attributes must use the I-variant to avoid float conversion.
            glVertexAttribIPointer(m_attribIndex,
                                   static_cast<GLint>(element.componentCount()),
                                   glBaseType(element.type),
                                   static_cast<GLsizei>(layout.stride()), offset);
        } else {
            glVertexAttribPointer(m_attribIndex,
                                  static_cast<GLint>(element.componentCount()),
                                  glBaseType(element.type),
                                  element.normalized ? GL_TRUE : GL_FALSE,
                                  static_cast<GLsizei>(layout.stride()), offset);
        }
        ++m_attribIndex;
    }

    m_vertexBuffers.push_back(std::move(buffer));
}

void VertexArray::setIndexBuffer(std::unique_ptr<IndexBuffer> buffer) {
    glBindVertexArray(m_id);
    buffer->bind(); // records this EBO into the VAO's element binding
    m_indexBuffer = std::move(buffer);
}

} // namespace ce
