#pragma once
#include <cstdint>
#include <functional>
#include <vector>

#include "core/math.h"
#include "core/obsptr.h"
#include "core/uuid.h"
#include "world/liquid.h"
#include "world/pos.h"

// these args are not permitted stay persistent.
// and, maybe in the future they might be moved into the dimension class.
// if you want to use them, it's better to pack like 'double get_air_rho(dimension* dim) { return ARC_AIR_RHO; }'
#define ARC_AIR_RHO 1.2
// I make the gravity weaker for flexibility.
#define ARC_GRAVITY_A 9.8
#define ARC_E_C_D 0.68
#define ARC_E_FUL 0.5
// unit: block
#define ARC_STEP_H 1.15

namespace arc {

namespace unit {

inline double to_meter(double v) { return v * 0.5; }
inline double to_meter2(double v) { return v * 0.25; }
inline double to_meter3(double v) { return v * 0.125; }
inline double to_meter_vol(const quad& box) { return std::pow(box.area(), 1.5); }
inline double to_block(double v) { return v * 2; }
inline double to_block2(double v) { return v * 4; }

}  // namespace unit

struct dimension;
struct chunk;

enum class entity_cat : uint8_t { none, creature, projectile, placement, particle };

namespace physic_ignore {

static const uint8_t none = 0U << 0;
static const uint8_t land_f = 1U << 0;
static const uint8_t liquid_f = 2U << 0;
static const uint8_t air_f = 3U << 0;

};  // namespace physic_ignore

namespace phybit {

static const uint16_t none = 1U << 0;
static const uint16_t touch_r = 1U << 1;
static const uint16_t touch_l = 1U << 2;
static const uint16_t touch_u = 1U << 3;
static const uint16_t touch_d = 1U << 4;
static const uint16_t touch_h = 1U << 5;
static const uint16_t touch_v = 1U << 6;
static const uint16_t touch_y = 1U << 7;
static const uint16_t swim = 1U << 8;

};  // namespace phybit

struct physic_obj {
    quad box;
    pos2d pos;
    pos2d prev_pos;
    vec2 velocity;
    vec2 face = vec2(0.0, 0.0);
    double mass = 1.0;
    double rotation = 0.0;
    double face_rot = 0.0;
    double rebound = 0.0;
    std::vector<pos2i> collision;
    std::unordered_map<liquid_behavior*, double> wetted_volume;
    bool is_spike = false;
    int16_t phy_status = phybit::none;
    int16_t prev_phy_status = phybit::none;
    bool net_dirty = true;
    bool server_auth = true;
    bool low_phy_sim = false;
    uint8_t ignore_lf_ = physic_ignore::none;

    void impulse(const vec2& imp);
    void impulse_limited(const vec2& imp);
    void move(const vec2& acc, const vec2& lim, uint8_t ig = physic_ignore::none);
    void locate(const pos2d& pos_);
    void locate_primary(const pos2d& pos_);
    quad lerped_box_();
};

struct entity : physic_obj {
    uuid uuid = uuid::make();
    obs<chunk> parent = nullptr;
    dimension* dim = nullptr;
    uint32_t wt_anc_ = 0;
    uint32_t ticks = 0;
    bool is_dead = false;
    float death_timer = 0.0;
    entity_cat cat;

    void motion();
    void force();
    std::function<void(entity* self)> tick;
    std::function<float(entity* self, int pipe)> cast_light;
};

}  // namespace arc