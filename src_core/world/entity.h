#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include "core/math.h"
#include "core/obsptr.h"
#include "core/uuid.h"
#include "world/pos.h"

namespace arc {

struct dimension;
struct chunk;

enum class entity_cat : uint8_t {
    none,
    creature,
    projectile,
    placement
};

enum class touch_bit : uint8_t {
    touch_n = 1U << 0,
    touch_r = 1U << 1,
    touch_l = 1U << 2,
    touch_u = 1U << 3,
    touch_d = 1U << 4,
    touch_h = 1U << 5,
    touch_v = 1U << 6,
    touch_y = 1U << 7
};

struct component_motion {
    quad aabb;
    pos2d pos;
    pos2d prev_pos;
    vec2 velocity;
    vec2 face;
    double mass = 1.0;
    double rotation = 0.0;
    std::vector<pos2i> collision;
    touch_bit touch = touch_bit::touch_n;
};

struct entity {
    uuid uuid = uuid::make();
    obs<chunk> parent = nullptr;
    dimension* dim = nullptr;
    uint32_t wt_anc_ = 0;
    uint32_t ticks = 0;
    bool is_dead = false;
    float death_timer = 0.0;
    component_motion* motion = nullptr;
    
    virtual void tick() {}
    std::function<float(entity* self, int pipe)> cast_light;
};

void entity_motion_component_handler_(dimension* dim, const uuid& e, component_motion& mot);

}  // namespace arc