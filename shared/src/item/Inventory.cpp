#include <cw/item/Inventory.h>

#include <net/ByteBuffer.h>

namespace cw {

uint8_t Inventory::add(ItemStack stack) {
    if (stack.empty()) return 0;
    const uint8_t cap = maxStack(stack.type);

    // Stackable items top up existing stacks first (tools never merge).
    if (cap > 1) {
        for (auto& s : m_slots) {
            if (s.type == stack.type && s.count < cap) {
                const uint8_t space = cap - s.count;
                const uint8_t moved = stack.count < space ? stack.count : space;
                s.count += moved;
                stack.count -= moved;
                if (stack.count == 0) return 0;
            }
        }
    }
    // Then fill empty slots.
    for (auto& s : m_slots) {
        if (s.type == ItemType::None || s.count == 0) {
            s.type       = stack.type;
            const uint8_t moved = stack.count < cap ? stack.count : cap;
            s.count      = moved;
            s.durability = stack.durability;
            stack.count -= moved;
            if (stack.count == 0) return 0;
        }
    }
    return stack.count; // leftover that didn't fit
}

ItemType Inventory::removeOne(int i) {
    if (i < 0 || i >= kSize) return ItemType::None;
    ItemStack& s = m_slots[i];
    if (s.empty()) return ItemType::None;
    const ItemType type = s.type;
    if (--s.count == 0) s.type = ItemType::None;
    return type;
}

int Inventory::count(ItemType type) const {
    int total = 0;
    for (const auto& s : m_slots)
        if (s.type == type) total += s.count;
    return total;
}

bool Inventory::removeItems(ItemType type, int n) {
    if (count(type) < n) return false;
    for (auto& s : m_slots) {
        if (n <= 0) break;
        if (s.type != type) continue;
        const int take = s.count < n ? s.count : n;
        s.count -= static_cast<uint8_t>(take);
        n -= take;
        if (s.count == 0) s.type = ItemType::None;
    }
    return true;
}

void Inventory::serialize(net::ByteBuffer& b) const {
    for (const auto& s : m_slots) {
        b.writeU16(static_cast<uint16_t>(s.type));
        b.writeU8(s.count);
        b.writeU16(s.durability);
    }
}

void Inventory::deserialize(net::ByteBuffer& b) {
    for (auto& s : m_slots) {
        s.type       = static_cast<ItemType>(b.readU16());
        s.count      = b.readU8();
        s.durability = b.readU16();
    }
}

} // namespace cw
