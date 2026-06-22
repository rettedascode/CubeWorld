#include <cw/craft/Crafting.h>

namespace cw {

const std::vector<CraftRecipe>& craftingRecipes() {
    static const std::vector<CraftRecipe> kRecipes = {
        // 4 sand -> 1 stone
        {{{ItemType::Sand, 4}}, {ItemType::Stone, 1}},
        // 1 stone -> 4 dirt
        {{{ItemType::Stone, 1}}, {ItemType::Dirt, 4}},
        // 3 stone -> 1 stone pickaxe
        {{{ItemType::Stone, 3}}, {ItemType::StonePickaxe, 1}},
    };
    return kRecipes;
}

bool tryCraft(Inventory& inv, int index) {
    const auto& recipes = craftingRecipes();
    if (index < 0 || index >= static_cast<int>(recipes.size())) return false;
    const CraftRecipe& r = recipes[index];

    // Verify all inputs are present before consuming any.
    for (const auto& in : r.inputs)
        if (inv.count(in.type) < in.count) return false;

    for (const auto& in : r.inputs) inv.removeItems(in.type, in.count);

    ItemStack out = r.output;
    if (isTool(out.type)) out.durability = static_cast<uint16_t>(toolDurability(out.type));
    inv.add(out);
    return true;
}

} // namespace cw
