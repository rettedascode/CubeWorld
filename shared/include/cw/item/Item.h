#pragma once

#include <cw/world/Block.h>

#include <cstdint>

namespace cw {

// Item ids. Block items share their numeric value with the matching BlockType so
// conversion is trivial. Tools get added in C4. SHARED — no rendering.
enum class ItemType : uint16_t {
    None         = 0,
    Grass        = 1,
    Dirt         = 2,
    Stone        = 3,
    Sand         = 4,
    Apple        = 5, // food
    StonePickaxe = 6, // tool (C4)
    Count
};

constexpr uint8_t kMaxStack = 64;

struct ItemStack {
    ItemType type       = ItemType::None;
    uint8_t  count      = 0;
    uint16_t durability = 0; // remaining uses for tools (0 for everything else)

    bool empty() const { return type == ItemType::None || count == 0; }
};

// True if the item places a block.
inline bool isBlockItem(ItemType t) {
    return t >= ItemType::Grass && t <= ItemType::Sand;
}

// Food: how much hunger the item restores (0 = not edible).
inline int foodValue(ItemType t) {
    return t == ItemType::Apple ? 6 : 0;
}
inline bool isFood(ItemType t) { return foodValue(t) > 0; }

// Tools (C4): mining-speed multiplier vs bare hands, and durability.
inline bool isTool(ItemType t) { return t == ItemType::StonePickaxe; }
inline float toolSpeed(ItemType t) { return t == ItemType::StonePickaxe ? 4.0f : 1.0f; }
inline int   toolDurability(ItemType t) { return t == ItemType::StonePickaxe ? 64 : 0; }

// Max stack size: tools are not stackable.
inline uint8_t maxStack(ItemType t) { return isTool(t) ? 1 : kMaxStack; }

// The block an item places (Air if it is not a block item).
inline BlockType blockForItem(ItemType t) {
    return isBlockItem(t) ? static_cast<BlockType>(t) : BlockType::Air;
}

// The item a broken block drops (None if it drops nothing, e.g. air/water).
inline ItemType itemForBlock(BlockType b) {
    switch (b) {
        case BlockType::Grass: return ItemType::Dirt; // grass drops dirt
        case BlockType::Dirt:  return ItemType::Dirt;
        case BlockType::Stone: return ItemType::Stone;
        case BlockType::Sand:  return ItemType::Sand;
        default:               return ItemType::None;
    }
}

} // namespace cw
