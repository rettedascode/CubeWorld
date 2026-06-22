#pragma once

#include <cw/item/Inventory.h>
#include <cw/item/Item.h>

#include <vector>

namespace cw {

// A shapeless recipe: a set of input stacks that produce one output stack.
// (The project asks for spatial 2x2/3x3 grids; we use a shapeless recipe-book
// model here — a deliberate simplification noted in the milestone summary.)
struct CraftRecipe {
    std::vector<ItemStack> inputs;
    ItemStack              output;
};

// The global recipe list (deterministic order; index is the recipe id over the net).
const std::vector<CraftRecipe>& craftingRecipes();

// Server-authoritative: if the inventory holds the inputs for recipe `index`,
// consume them, add the output, and return true.
bool tryCraft(Inventory& inv, int index);

} // namespace cw
