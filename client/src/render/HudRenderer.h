#pragma once

#include <ce/render/Camera.h>

#include <memory>
#include <string>
#include <vector>

namespace ce {
class Shader;
class Mesh;
class Texture2D;
} // namespace ce

namespace cw {

class Inventory;

// CLIENT-only 2D HUD: draws the hotbar (9 slots) in screen space using an
// orthographic projection, sampling block-item icons from the atlas. No text
// counts yet (the engine has no text renderer).
class HudRenderer {
public:
    HudRenderer();
    ~HudRenderer();

    void load(const std::string& assetDir);
    void render(ce::Texture2D& atlas, const Inventory& inv, int selectedSlot,
                float health, float food, uint32_t screenW, uint32_t screenH);

    // Crafting overlay: draws the recipe list; returns the recipe index under the
    // cursor (top-left origin) for click handling, or -1.
    void renderCrafting(ce::Texture2D& atlas, uint32_t screenW, uint32_t screenH);
    int  craftHitTest(float mouseX, float mouseY, uint32_t screenW, uint32_t screenH);

    // Text overlays (use the bitmap font).
    void renderChat(const std::vector<std::string>& lines, const std::string& input,
                    bool inputActive, uint32_t screenW, uint32_t screenH);
    void renderPlayerList(const std::vector<std::string>& names,
                          uint32_t screenW, uint32_t screenH);

private:
    void drawQuad(float x, float y, float w, float h, bool useTexture,
                  const glm::vec4& tint, glm::vec2 uvMin, glm::vec2 uvMax);
    void beginUI(uint32_t screenW, uint32_t screenH);
    void drawText(const std::string& text, float x, float y, float scale,
                  const glm::vec4& color); // requires font bound + beginUI

    ce::Camera                     m_camera;
    std::unique_ptr<ce::Shader>    m_shader;
    std::unique_ptr<ce::Mesh>      m_quad;
    std::unique_ptr<ce::Texture2D> m_font;
};

} // namespace cw
