#pragma once

#include <glm/glm.hpp>

namespace cw {

class World;

struct RaycastHit {
    bool       hit    = false;
    glm::ivec3 block{0};  // the solid block that was hit
    glm::ivec3 normal{0}; // face normal pointing out of that block (toward the ray)
};

// Amanatides & Woo voxel traversal: marches a ray through the voxel grid one
// cell at a time and returns the first opaque block within maxDistance, along
// with the face normal entered through. `block + normal` is the empty cell to
// place a new block into.
RaycastHit raycastVoxel(const World& world, const glm::vec3& origin,
                        const glm::vec3& direction, float maxDistance);

} // namespace cw
