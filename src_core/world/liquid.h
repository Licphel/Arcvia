#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>

#include "core/math.h"
#include "world/group.h"
#include "world/pos.h"

namespace arc {

struct dimension;
struct liquid_behavior;

struct liquid_stack {
    static uint8_t max_amount;
    liquid_behavior* liquid;
    uint8_t amount;

    bool is_empty();
    bool is_full();
    bool is(liquid_behavior* liquid_);
};

enum liquid_tick_mode { random, consistent };

struct liquid_behavior : group {
    //ARC_REGISTERABLE
    liquid_tick_mode tick_mode = liquid_tick_mode::random;
    double stickness = 1.0;
    double density = 1.0;
    bool is_gas = false; //todo

    bool contains(const location& loc) const override;

    std::function<float(dimension* dim, const pos2i& pos, liquid_stack& s, int pipe)> cast_light;
    std::function<float(dimension* dim, const pos2i& pos, liquid_stack& s, int pipe, float val)> block_light;
    std::function<quad(dimension* dim, const pos2i& pos, liquid_stack& s)> render_space;
    std::function<void(dimension* dim, const pos2i& pos, liquid_stack& s, const pos2i& pos_other, liquid_stack& s_other)> on_touch;
    std::function<void(dimension* dim, const pos2i& pos, liquid_stack& s)> tick;
};

}  // namespace arc