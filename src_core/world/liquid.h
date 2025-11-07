#pragma once
#include <cstddef>
#include <cstdint>

#include "core/math.h"
#include "core/obsptr.h"
#include "core/prop.h"
#include "world/group.h"
#include "world/pos.h"

namespace arc {

struct dimension;
struct liquid_behavior;
struct liquid_model;
struct entity;
struct chunk;

struct liquid_stack {
    static uint8_t max_amount;
    liquid_behavior* liquid;
    uint8_t amount;

    bool is_empty() const;
    bool is_full() const;
    bool is(liquid_behavior* liquid_) const;
    double percentage() const;
};

enum liquid_tick_mode { random, consistent };

struct liquid_behavior : group {
    // ARC_REGISTERABLE
    liquid_tick_mode tick_mode = liquid_tick_mode::random;
    liquid_model* model;

    bool contains(const location& loc) const override;

    property<float(dimension* dim, const pos2i& pos, liquid_stack& s, int pipe)> cast_light = 0.0;
    property<float(dimension* dim, const pos2i& pos, liquid_stack& s, int pipe, float val)> block_light;
    property<quad(dimension* dim, const pos2i& pos, liquid_stack& s)> render_space;
    property<void(dimension* dim, const pos2i& pos, liquid_stack& s, const pos2i& pos_other, liquid_stack& s_other)>
        on_touch;
    property<void(obs<entity> e)> entity_collide;
    property<void(dimension* dim, const pos2i& pos, liquid_stack& s)> tick;
    property<double(dimension* dim)> stickiness = 1000.0;
    property<double(dimension* dim)> density = 1000.0;
    property<double(dimension* dim)> is_gas = false;
};

struct liquid_flow_engine {
    liquid_flow_engine(obs<chunk> chunk_);
};

}  // namespace arc