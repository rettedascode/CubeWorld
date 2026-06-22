#include "render/EntityRenderer.h"

#include <ce/render/BufferLayout.h>
#include <ce/render/Mesh.h>
#include <ce/render/Renderer.h>
#include <ce/render/Shader.h>

#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>
#include <iterator>

namespace cw {

namespace {
// Unit cube centred at the origin (positions only). Face culling is disabled
// while drawing entities, so winding does not matter here.
const float kCubeVertices[] = {
    -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, 0.5f, -0.5f,  -0.5f, 0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, 0.5f,  0.5f,  -0.5f, 0.5f,  0.5f,
};
const uint32_t kCubeIndices[] = {
    0, 1, 2, 2, 3, 0,  4, 5, 6, 6, 7, 4,  0, 4, 7, 7, 3, 0,
    1, 5, 6, 6, 2, 1,  3, 2, 6, 6, 7, 3,  0, 1, 5, 5, 4, 0,
};
// Placeholder proportions/colours per entity type.
const glm::vec3 kPlayerScale{0.6f, 1.8f, 0.6f};
const glm::vec3 kItemScale{0.3f, 0.3f, 0.3f};
const glm::vec3 kMobScale{0.8f, 1.2f, 0.8f};
const glm::vec4 kPlayerColor{0.85f, 0.2f, 0.2f, 1.0f};
const glm::vec4 kItemColor{0.95f, 0.85f, 0.2f, 1.0f};
const glm::vec4 kPassiveColor{0.9f, 0.9f, 0.9f, 1.0f};  // sheep-ish
const glm::vec4 kHostileColor{0.2f, 0.7f, 0.3f, 1.0f};  // zombie-ish
} // namespace

EntityRenderer::EntityRenderer()  = default;
EntityRenderer::~EntityRenderer() = default;

void EntityRenderer::load(const std::string& assetDir) {
    const ce::BufferLayout layout = {{ce::ShaderDataType::Float3, "a_position"}};
    m_cube = std::make_unique<ce::Mesh>(kCubeVertices,
                                        static_cast<uint32_t>(sizeof(kCubeVertices)),
                                        kCubeIndices,
                                        static_cast<uint32_t>(std::size(kCubeIndices)),
                                        layout);
    m_shader = ce::Shader::fromFiles(assetDir + "/shaders/entity.vert",
                                     assetDir + "/shaders/entity.frag");
}

void EntityRenderer::render(const std::unordered_map<EntityId, RemoteEntity>& entities) {
    if (entities.empty() || !m_shader || !m_cube) return;

    ce::Renderer::setFaceCulling(false); // placeholder cube has arbitrary winding
    m_shader->bind();

    for (const auto& [id, e] : entities) {
        const bool isItem = e.type == static_cast<uint8_t>(EntityType::Item);
        const bool isMob  = e.type == static_cast<uint8_t>(EntityType::Mob);

        glm::vec4 color = kPlayerColor;
        glm::vec3 scale = kPlayerScale;
        if (isItem) { color = kItemColor; scale = kItemScale; }
        else if (isMob) {
            color = e.hostile ? kHostileColor : kPassiveColor;
            scale = kMobScale;
        }
        m_shader->setFloat4("u_color", color);

        glm::mat4 t = glm::translate(glm::mat4(1.0f), e.position);
        t           = glm::rotate(t, e.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        t           = glm::scale(t, scale);
        ce::Renderer::submit(*m_shader, *m_cube, t);
    }

    ce::Renderer::setFaceCulling(true);
}

} // namespace cw
