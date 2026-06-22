#include "ce/render/Mesh.h"

namespace ce {

Mesh::Mesh(const void* vertexData, uint32_t vertexBytes,
           const uint32_t* indices, uint32_t indexCount,
           const BufferLayout& layout) {
    m_vao = std::make_unique<VertexArray>();

    auto vb = std::make_unique<VertexBuffer>(vertexData, vertexBytes);
    vb->setLayout(layout);
    m_vao->addVertexBuffer(std::move(vb));

    m_vao->setIndexBuffer(std::make_unique<IndexBuffer>(indices, indexCount));
}

uint32_t Mesh::indexCount() const {
    const IndexBuffer* ib = m_vao->indexBuffer();
    return ib ? ib->count() : 0;
}

} // namespace ce
