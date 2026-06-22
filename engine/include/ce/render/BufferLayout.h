#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Describes the layout of a vertex buffer so the engine can configure a
// VertexArray for ANY vertex format the game supplies. The engine itself never
// hardcodes a "block vertex" — the game builds a BufferLayout and hands it over.
namespace ce {

enum class ShaderDataType {
    None = 0,
    Float, Float2, Float3, Float4,
    Int, Int2, Int3, Int4,
    Bool
};

inline uint32_t shaderDataTypeSize(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:  return 4;
        case ShaderDataType::Float2: return 4 * 2;
        case ShaderDataType::Float3: return 4 * 3;
        case ShaderDataType::Float4: return 4 * 4;
        case ShaderDataType::Int:    return 4;
        case ShaderDataType::Int2:   return 4 * 2;
        case ShaderDataType::Int3:   return 4 * 3;
        case ShaderDataType::Int4:   return 4 * 4;
        case ShaderDataType::Bool:   return 1;
        default:                     return 0;
    }
}

struct BufferElement {
    std::string    name;
    ShaderDataType type       = ShaderDataType::None;
    uint32_t       size       = 0;
    uint32_t       offset     = 0;
    bool           normalized = false;

    BufferElement() = default;
    BufferElement(ShaderDataType t, std::string n, bool norm = false)
        : name(std::move(n)), type(t), size(shaderDataTypeSize(t)), normalized(norm) {}

    // Number of scalar components (e.g. Float3 -> 3) for glVertexAttribPointer.
    uint32_t componentCount() const {
        switch (type) {
            case ShaderDataType::Float:  return 1;
            case ShaderDataType::Float2: return 2;
            case ShaderDataType::Float3: return 3;
            case ShaderDataType::Float4: return 4;
            case ShaderDataType::Int:    return 1;
            case ShaderDataType::Int2:   return 2;
            case ShaderDataType::Int3:   return 3;
            case ShaderDataType::Int4:   return 4;
            case ShaderDataType::Bool:   return 1;
            default:                     return 0;
        }
    }
};

class BufferLayout {
public:
    BufferLayout() = default;
    BufferLayout(std::initializer_list<BufferElement> elements)
        : m_elements(elements) {
        calculateOffsetsAndStride();
    }

    uint32_t stride() const { return m_stride; }
    const std::vector<BufferElement>& elements() const { return m_elements; }

    std::vector<BufferElement>::const_iterator begin() const { return m_elements.begin(); }
    std::vector<BufferElement>::const_iterator end()   const { return m_elements.end(); }

private:
    void calculateOffsetsAndStride() {
        uint32_t offset = 0;
        m_stride = 0;
        for (auto& e : m_elements) {
            e.offset = offset;
            offset  += e.size;
            m_stride += e.size;
        }
    }

    std::vector<BufferElement> m_elements;
    uint32_t                   m_stride = 0;
};

} // namespace ce
