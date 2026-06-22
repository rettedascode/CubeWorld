#pragma once

#include <cw/item/Item.h>

#include <array>

namespace net { class ByteBuffer; }

namespace cw {

// Player inventory: a 9-slot hotbar followed by a 27-slot main store (36 total).
// SHARED, server-authoritative. The client keeps a mirror for the HUD.
class Inventory {
public:
    static constexpr int kHotbar = 9;
    static constexpr int kMain   = 27;
    static constexpr int kSize   = kHotbar + kMain;

    ItemStack&       slot(int i) { return m_slots[i]; }
    const ItemStack& slot(int i) const { return m_slots[i]; }

    // Add a stack; returns the count that did not fit (0 if all added).
    uint8_t add(ItemStack stack);

    // Remove one item from a slot (used when placing). Returns the item removed,
    // or None if the slot was empty.
    ItemType removeOne(int i);

    // Crafting helpers.
    int  count(ItemType type) const;        // total of an item across all slots
    bool removeItems(ItemType type, int n); // remove n if available; false otherwise

    void serialize(net::ByteBuffer& b) const;
    void deserialize(net::ByteBuffer& b);

private:
    std::array<ItemStack, kSize> m_slots;
};

} // namespace cw
