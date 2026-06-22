#pragma once

#include "ce/render/Camera.h"

#include <cstdint>
#include <glm/glm.hpp>

namespace ce {

class Shader;
class VertexArray;
class Mesh;

// Thin immediate-mode renderer. Owns global GL state (clear color, depth test,
// face culling) and the begin/submit/end scene flow. All raw GL lives here or
// in the resource wrappers — the game never calls GL directly.
class Renderer {
public:
    static void init();   // baseline GL state; call once after the GL context exists
    static void setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    static void setClearColor(const glm::vec4& color);
    static void clear();
    static void setDepthTest(bool enabled);
    static void setFaceCulling(bool enabled); // culls back faces when enabled

    static void beginScene(const Camera& camera);
    static void submit(Shader& shader, const VertexArray& va,
                       const glm::mat4& transform = glm::mat4(1.0f));
    static void submit(Shader& shader, const Mesh& mesh,
                       const glm::mat4& transform = glm::mat4(1.0f));
    static void endScene();

private:
    static glm::mat4 s_viewProjection;
};

} // namespace ce
