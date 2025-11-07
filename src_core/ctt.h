#pragma once
#include "core/reg.h"
#include "world/group.h"

namespace arc {

struct block_behavior;
struct block_model;
struct liquid_behavior;
struct liquid_model;
struct item_behavior;
struct group;
struct group_map;

extern template class registry<block_behavior>;
extern template class registry<block_model>;
extern template class registry<liquid_behavior>;
extern template class registry<liquid_model>;
extern template class registry<item_behavior>;
extern template class registry<group>;
extern template class registry<group_map>;

inline block_behavior* block_void;
inline block_model* block_model_void;
inline item_behavior* item_void;
inline liquid_behavior* liquid_void;
inline liquid_model* liquid_model_void;

void R_make_nonnulls();

registry<block_behavior>& R_blocks();
registry<block_model>& R_block_models();
registry<liquid_behavior>& R_liquids();
registry<liquid_model>& R_liquid_models();
registry<item_behavior>& R_items();
registry<group_flat>& R_groups();
registry<group_map>& R_group_maps();

}  // namespace arc