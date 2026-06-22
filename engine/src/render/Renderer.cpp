#include "ce/render/Renderer.h"

#include "ce/render/Mesh.h"
#include "ce/render/Shader.h"
#include "ce/render/VertexArray.h"

#include <glad/glad.h>

namespace ce {

glm::mat4 Renderer::s_viewProjection{1.0f};

void Renderer::init() {
    glEnable(GL_DEPTH_TEST);

    // Standard alpha blending, useful for textured quads with transparency.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Counter-clockwise winding = front face (our convention).
    glFrontFace(GL_CCW);
    glCullFace(GL_BACK);
}

void Renderer::setViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    glViewport(static_cast<GLint>(x), static_cast<GLint>(y),
               static_cast<GLsizei>(width), static_cast<GLsizei>(height));
}

void Renderer::setClearColor(const glm::vec4& color) {
    glClearColor(color.r, color.g, color.b, color.a);
}

void Renderer::clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::setDepthTest(bool enabled) {
    if (enabled) glEnable(GL_DEPTH_TEST);
    else         glDisable(GL_DEPTH_TEST);
}

void Renderer::setFaceCulling(bool enabled) {
    if (enabled) glEnable(GL_CULL_FACE);
    else         glDisable(GL_CULL_FACE);
}

void Renderer::beginScene(const Camera& camera) {
    s_viewProjection = camera.viewProjection();
}

void Renderer::submit(Shader& shader, const VertexArray& va, const glm::mat4& transform) {
    shader.bind();
    shader.setMat4("u_viewProjection", s_viewProjection);
    shader.setMat4("u_transform", transform);

    va.bind();
    const IndexBuffer* ib = va.indexBuffer();
    const GLsizei count = ib ? static_cast<GLsizei>(ib->count()) : 0;
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
}

void Renderer::submit(Shader& shader, const Mesh& mesh, const glm::mat4& transform) {
    submit(shader, mesh.vertexArray(), transform);
}

void Renderer::endScene() {
    // Nothing batched yet; present happens via Window::onUpdate.
}

} // namespace ce
