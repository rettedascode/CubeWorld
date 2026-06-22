#pragma once

#include "ce/render/BufferLayout.h"
#include "ce/render/VertexArray.h"

#include <cstdint>
#include <memory>

namespace ce {

// Uploads interleaved vertex data + indices to the GPU as a single drawable
// unit. The layout is supplied by the caller, so the engine stays agnostic
// about what a vertex contains.
class Mesh {
public:
    Mesh(const void* vertexData, uint32_t vertexBytes,
         const uint32_t* indices, uint32_t indexCount,
         const BufferLayout& layout);

    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;

    const VertexArray& vertexArray() const { return *m_vao; }
    uint32_t indexCount() const;
    bool empty() const { return indexCount() == 0; }

private:
    std::unique_ptr<VertexArray> m_vao;
};

} // namespace ce
