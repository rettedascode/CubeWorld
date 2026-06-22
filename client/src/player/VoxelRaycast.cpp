#include "player/VoxelRaycast.h"

#include <cw/world/World.h>

#include <cmath>
#include <limits>

namespace cw {

RaycastHit raycastVoxel(const World& world, const glm::vec3& origin,
                        const glm::vec3& direction, float maxDistance) {
    RaycastHit result;

    glm::vec3   d   = direction;
    const float len = glm::length(d);
    if (len < 1e-6f) return result;
    d /= len; // normalise so t is measured in world units

    // Current voxel.
    int px = static_cast<int>(std::floor(origin.x));
    int py = static_cast<int>(std::floor(origin.y));
    int pz = static_cast<int>(std::floor(origin.z));

    const int stepX = (d.x > 0) ? 1 : (d.x < 0 ? -1 : 0);
    const int stepY = (d.y > 0) ? 1 : (d.y < 0 ? -1 : 0);
    const int stepZ = (d.z > 0) ? 1 : (d.z < 0 ? -1 : 0);

    const float INF = std::numeric_limits<float>::infinity();

    // Ray parameter t at which we first cross each axis boundary.
    auto firstBoundary = [INF](int p, int step, float o, float dd) -> float {
        if (step == 0) return INF;
        const float next = (step > 0) ? static_cast<float>(p + 1)
                                      : static_cast<float>(p);
        return (next - o) / dd;
    };
    float tMaxX = firstBoundary(px, stepX, origin.x, d.x);
    float tMaxY = firstBoundary(py, stepY, origin.y, d.y);
    float tMaxZ = firstBoundary(pz, stepZ, origin.z, d.z);

    // Ray parameter delta to cross a full voxel along each axis.
    const float tDeltaX = (stepX != 0) ? std::fabs(1.0f / d.x) : INF;
    const float tDeltaY = (stepY != 0) ? std::fabs(1.0f / d.y) : INF;
    const float tDeltaZ = (stepZ != 0) ? std::fabs(1.0f / d.z) : INF;

    glm::ivec3 normal{0};
    float      t = 0.0f;

    for (int i = 0; i < 1024; ++i) {
        if (isOpaque(world.getBlock(px, py, pz))) {
            result.hit    = true;
            result.block  = {px, py, pz};
            result.normal = normal; // face we entered through
            return result;
        }
        if (t > maxDistance) break;

        // Advance into the next voxel across the nearest boundary.
        if (tMaxX <= tMaxY && tMaxX <= tMaxZ) {
            px += stepX; t = tMaxX; tMaxX += tDeltaX; normal = {-stepX, 0, 0};
        } else if (tMaxY <= tMaxZ) {
            py += stepY; t = tMaxY; tMaxY += tDeltaY; normal = {0, -stepY, 0};
        } else {
            pz += stepZ; t = tMaxZ; tMaxZ += tDeltaZ; normal = {0, 0, -stepZ};
        }
    }

    return result; // no hit within range
}

} // namespace cw
