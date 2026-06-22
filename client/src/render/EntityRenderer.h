#pragma once

#include "net/NetClient.h"

#include <cw/entity/Entity.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace ce {
class Shader;
class Mesh;
} // namespace ce

namespace cw {

// CLIENT-only: draws remote entities as flat-coloured placeholder cubes. Assumes
// Renderer::beginScene() has already been called for the frame.
class EntityRenderer {
public:
    EntityRenderer();
    ~EntityRenderer();

    void load(const std::string& assetDir);
    void render(const std::unordered_map<EntityId, RemoteEntity>& entities);

private:
    std::unique_ptr<ce::Shader> m_shader;
    std::unique_ptr<ce::Mesh>   m_cube;
};

} // namespace cw
