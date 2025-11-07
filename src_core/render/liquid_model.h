#pragma once
#include <cstdint>
#include <memory>

#include "gfx/brush.h"
#include "gfx/image.h"
#include "world/block.h"
#include "world/liquid.h"

namespace arc {

struct liquid_model {
    ARC_REGISTERABLE
    void (*make_liquid)(brush* brush, liquid_model* self, dimension* dim, const liquid_stack& qstack,
                        const pos2d& pos) = default_make_liquid_;
    std::shared_ptr<texture> tex = nullptr;
    std::shared_ptr<texture> tex_edge = nullptr;
    double flow_speed = 1.0;

    static void default_make_liquid_(brush* brush, liquid_model* self, dimension* dim, const liquid_stack& qstack,
                                     const pos2d& pos);
};

}  // namespace arc