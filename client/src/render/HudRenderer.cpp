#include "render/HudRenderer.h"

#include "render/BlockTextures.h"

#include <cw/craft/Crafting.h>
#include <cw/item/Inventory.h>
#include <cw/item/Item.h>

#include <ce/render/BufferLayout.h>
#include <ce/render/Mesh.h>
#include <ce/render/Renderer.h>
#include <ce/render/Shader.h>
#include <ce/render/Texture.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdint>

namespace cw {

namespace {
constexpr float kSlot = 50.0f; // slot size in pixels
constexpr float kPad  = 6.0f;  // gap between slots
constexpr float kMargin = 24.0f;

// Crafting recipe row rect (top-left origin), kept identical for draw + hit-test.
void craftRectTopLeft(int i, uint32_t screenW, float& x, float& yTop, float& size) {
    size = 56.0f;
    x    = screenW * 0.5f - size * 0.5f;
    yTop = 120.0f + i * (size + 10.0f);
}

// Atlas UV sub-rect for a tile (matches ChunkMesher: v flipped, half-texel inset).
void tileUV(int tile, glm::vec2& uvMin, glm::vec2& uvMax) {
    const float span  = 1.0f / kAtlasCols;
    const float inset = 0.5f / (kAtlasCols * kAtlasTilePixels);
    const int   col   = tile % kAtlasCols;
    const int   row   = tile / kAtlasCols;
    const float u0 = col * span + inset;
    const float u1 = (col + 1) * span - inset;
    const float vTop = 1.0f - row * span;
    const float vBot = 1.0f - (row + 1) * span;
    uvMin = {u0, vBot + inset};
    uvMax = {u1, vTop - inset};
}
} // namespace

HudRenderer::HudRenderer()  = default;
HudRenderer::~HudRenderer() = default;

void HudRenderer::load(const std::string& assetDir) {
    const float quad[] = {
        0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
    };
    const uint32_t idx[] = {0, 1, 2, 2, 3, 0};
    const ce::BufferLayout layout = {{ce::ShaderDataType::Float3, "a_position"},
                                     {ce::ShaderDataType::Float2, "a_texCoord"}};
    m_quad = std::make_unique<ce::Mesh>(quad, static_cast<uint32_t>(sizeof(quad)),
                                        idx, 6, layout);
    m_shader = ce::Shader::fromFiles(assetDir + "/shaders/hud.vert",
                                     assetDir + "/shaders/hud.frag");
    m_font   = std::make_unique<ce::Texture2D>(assetDir + "/textures/font.png");
}

void HudRenderer::beginUI(uint32_t screenW, uint32_t screenH) {
    m_camera.setOrthographic(0.0f, static_cast<float>(screenW),
                             0.0f, static_cast<float>(screenH));
    ce::Renderer::setDepthTest(false);
    ce::Renderer::setFaceCulling(false);
    ce::Renderer::beginScene(m_camera);
    m_shader->bind();
    m_shader->setInt("u_texture", 0);
}

void HudRenderer::drawText(const std::string& text, float x, float y, float scale,
                           const glm::vec4& color) {
    // Font atlas: 16x6 cells of 8x8 px; each glyph is the top-left 5x7 of its cell.
    constexpr float kAtlasW = 128.0f, kAtlasH = 48.0f;
    float cx = x;
    for (char raw : text) {
        const unsigned char ch = static_cast<unsigned char>(raw);
        if (ch > 32 && ch < 128) {
            const int idx  = ch - 32;
            const int col  = idx % 16;
            const int row  = idx / 16;
            const float px = col * 8.0f, py = row * 8.0f;
            const glm::vec2 uvMin{px / kAtlasW, 1.0f - (py + 7.0f) / kAtlasH};
            const glm::vec2 uvMax{(px + 5.0f) / kAtlasW, 1.0f - py / kAtlasH};
            drawQuad(cx, y, 5.0f * scale, 7.0f * scale, true, color, uvMin, uvMax);
        }
        cx += 6.0f * scale; // advance (space falls through here)
    }
}

void HudRenderer::renderChat(const std::vector<std::string>& lines, const std::string& input,
                             bool inputActive, uint32_t screenW, uint32_t screenH) {
    beginUI(screenW, screenH);
    m_font->bind(0);

    constexpr float scale = 2.0f;
    const float lineH = 9.0f * scale;
    float y = 110.0f; // just above the hotbar/bars

    if (inputActive) {
        drawText("> " + input + "_", 12.0f, 84.0f, scale, {1, 1, 0.6f, 1});
    }

    const int maxLines = 6;
    const int first    = static_cast<int>(lines.size()) - maxLines;
    for (int i = static_cast<int>(lines.size()) - 1; i >= 0 && i >= first; --i) {
        drawText(lines[i], 12.0f, y, scale, {1, 1, 1, 1});
        y += lineH;
    }
    ce::Renderer::setDepthTest(true);
    ce::Renderer::setFaceCulling(true);
}

void HudRenderer::renderPlayerList(const std::vector<std::string>& names,
                                   uint32_t screenW, uint32_t screenH) {
    beginUI(screenW, screenH);

    const glm::vec2 z{0, 0};
    const float w = 240.0f, h = 40.0f + names.size() * 22.0f;
    const float x = screenW * 0.5f - w * 0.5f;
    const float yTop = 80.0f;
    const float y = screenH - yTop - h;
    drawQuad(x, y, w, h, false, {0.0f, 0.0f, 0.0f, 0.6f}, z, z);

    m_font->bind(0);
    drawText("PLAYERS", x + 12.0f, screenH - yTop - 26.0f, 2.0f, {1, 1, 0.6f, 1});
    float ty = screenH - yTop - 50.0f;
    for (const auto& n : names) {
        drawText(n, x + 12.0f, ty, 2.0f, {1, 1, 1, 1});
        ty -= 22.0f;
    }
    ce::Renderer::setDepthTest(true);
    ce::Renderer::setFaceCulling(true);
}

void HudRenderer::drawQuad(float x, float y, float w, float h, bool useTexture,
                           const glm::vec4& tint, glm::vec2 uvMin, glm::vec2 uvMax) {
    m_shader->setInt("u_useTexture", useTexture ? 1 : 0);
    m_shader->setFloat4("u_tint", tint);
    m_shader->setFloat2("u_uvMin", uvMin);
    m_shader->setFloat2("u_uvMax", uvMax);
    glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, 0.0f));
    t           = glm::scale(t, glm::vec3(w, h, 1.0f));
    ce::Renderer::submit(*m_shader, *m_quad, t);
}

void HudRenderer::render(ce::Texture2D& atlas, const Inventory& inv, int selectedSlot,
                         float health, float food, uint32_t screenW, uint32_t screenH) {
    if (!m_shader || !m_quad) return;

    m_camera.setOrthographic(0.0f, static_cast<float>(screenW),
                             0.0f, static_cast<float>(screenH));

    ce::Renderer::setDepthTest(false);
    ce::Renderer::setFaceCulling(false);
    ce::Renderer::beginScene(m_camera);

    m_shader->bind();
    m_shader->setInt("u_texture", 0);
    atlas.bind(0);

    const float totalW = Inventory::kHotbar * kSlot + (Inventory::kHotbar - 1) * kPad;
    const float x0     = (screenW - totalW) * 0.5f;
    const float y0     = kMargin;

    // Health (red) and food (orange) bars just above the hotbar.
    const float barW = totalW;
    const float barH = 10.0f;
    const float hY   = y0 + kSlot + 10.0f;
    const float fY   = hY + barH + 4.0f;
    const glm::vec2 z{0, 0};
    drawQuad(x0, hY, barW, barH, false, {0.0f, 0.0f, 0.0f, 0.6f}, z, z);
    drawQuad(x0, hY, barW * glm::clamp(health / 20.0f, 0.0f, 1.0f), barH, false,
             {0.85f, 0.15f, 0.15f, 0.95f}, z, z);
    drawQuad(x0, fY, barW, barH, false, {0.0f, 0.0f, 0.0f, 0.6f}, z, z);
    drawQuad(x0, fY, barW * glm::clamp(food / 20.0f, 0.0f, 1.0f), barH, false,
             {0.85f, 0.55f, 0.15f, 0.95f}, z, z);

    for (int i = 0; i < Inventory::kHotbar; ++i) {
        const float x = x0 + i * (kSlot + kPad);

        const bool      selected = (i == selectedSlot);
        const glm::vec4 bg = selected ? glm::vec4(0.95f, 0.9f, 0.4f, 0.55f)
                                      : glm::vec4(0.1f, 0.1f, 0.1f, 0.55f);
        drawQuad(x, y0, kSlot, kSlot, false, bg, {0, 0}, {0, 0});

        const ItemStack st = inv.slot(i);
        if (isBlockItem(st.type)) {
            glm::vec2 uvMin, uvMax;
            tileUV(blockTextures(blockForItem(st.type)).top, uvMin, uvMax);
            const float inset = 6.0f;
            drawQuad(x + inset, y0 + inset, kSlot - 2 * inset, kSlot - 2 * inset, true,
                     {1, 1, 1, 1}, uvMin, uvMax);
        }
    }

    ce::Renderer::setDepthTest(true);
    ce::Renderer::setFaceCulling(true);
}

void HudRenderer::renderCrafting(ce::Texture2D& atlas, uint32_t screenW, uint32_t screenH) {
    if (!m_shader || !m_quad) return;

    m_camera.setOrthographic(0.0f, static_cast<float>(screenW),
                             0.0f, static_cast<float>(screenH));
    ce::Renderer::setDepthTest(false);
    ce::Renderer::setFaceCulling(false);
    ce::Renderer::beginScene(m_camera);
    m_shader->bind();
    m_shader->setInt("u_texture", 0);
    atlas.bind(0);

    const glm::vec2 z{0, 0};
    // Dim backdrop.
    drawQuad(0, 0, static_cast<float>(screenW), static_cast<float>(screenH), false,
             {0.0f, 0.0f, 0.0f, 0.5f}, z, z);

    const auto& recipes = craftingRecipes();
    for (int i = 0; i < static_cast<int>(recipes.size()); ++i) {
        float x, yTop, size;
        craftRectTopLeft(i, screenW, x, yTop, size);
        const float y = screenH - yTop - size; // to bottom-origin

        drawQuad(x, y, size, size, false, {0.15f, 0.15f, 0.15f, 0.9f}, z, z);

        const ItemType out = recipes[i].output.type;
        const float    in  = 8.0f;
        if (isBlockItem(out)) {
            glm::vec2 uvMin, uvMax;
            tileUV(blockTextures(blockForItem(out)).top, uvMin, uvMax);
            drawQuad(x + in, y + in, size - 2 * in, size - 2 * in, true, {1, 1, 1, 1},
                     uvMin, uvMax);
        } else {
            // Non-block output (e.g. tool): a distinct flat colour.
            drawQuad(x + in, y + in, size - 2 * in, size - 2 * in, false,
                     {0.6f, 0.6f, 0.7f, 1.0f}, z, z);
        }
    }

    ce::Renderer::setDepthTest(true);
    ce::Renderer::setFaceCulling(true);
}

int HudRenderer::craftHitTest(float mouseX, float mouseY, uint32_t screenW, uint32_t screenH) {
    (void)screenH;
    const auto& recipes = craftingRecipes();
    for (int i = 0; i < static_cast<int>(recipes.size()); ++i) {
        float x, yTop, size;
        craftRectTopLeft(i, screenW, x, yTop, size);
        if (mouseX >= x && mouseX <= x + size && mouseY >= yTop && mouseY <= yTop + size)
            return i;
    }
    return -1;
}

} // namespace cw
