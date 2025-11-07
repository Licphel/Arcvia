#include "ctt.h"

#include "render/block_model.h"
#include "render/liquid_model.h"
#include "world/block.h"
#include "world/group.h"
#include "world/item.h"
#include "world/liquid.h"

namespace arc {

template class registry<block_behavior>;
template class registry<block_model>;
template class registry<liquid_behavior>;
template class registry<liquid_model>;
template class registry<item_behavior>;
template class registry<group_flat>;
template class registry<group_map>;

void R_make_nonnulls() {
    // block
    block_void = R_blocks().make_nonnull("void", {});
    block_model_void = R_block_models().make_nonnull("void", {});
    block_void->model = block_model_void;
    // liquid
    liquid_void = R_liquids().make_nonnull("void", {});
    liquid_model_void = R_liquid_models().make_nonnull("void", {});
    liquid_void->model = liquid_model_void;
    // item
    item_void = R_items().make_nonnull("void", {});
}

registry<block_behavior>& R_blocks() {
    static registry<block_behavior> reg_;
    return reg_;
}

registry<block_model>& R_block_models() {
    static registry<block_model> reg_;
    return reg_;
}

registry<liquid_behavior>& R_liquids() {
    static registry<liquid_behavior> reg_;
    return reg_;
}

registry<liquid_model>& R_liquid_models() {
    static registry<liquid_model> reg_;
    return reg_;
}

registry<item_behavior>& R_items() {
    static registry<item_behavior> reg_;
    return reg_;
}

registry<group_flat>& R_groups() {
    static registry<group_flat> reg_;
    return reg_;
}

registry<group_map>& R_group_maps() {
    static registry<group_map> reg_;
    return reg_;
}

}  // namespace arc