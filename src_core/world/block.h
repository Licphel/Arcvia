#pragma once
#include <cstddef>
#include <functional>
#include <vector>

#include "core/codec.h"
#include "core/math.h"
#include "world/entity.h"
#include "world/group.h"
#include "world/pos.h"

namespace arc {

struct item_stack;
struct dimension;

enum class block_hierarchy : uint8_t { low, high };
enum class block_tick_mode : uint8_t { random, consistent };

struct block_shape {
    bool solid;
    float (*block_light)(float v);

    bool operator==(const block_shape& v);
    bool operator!=(const block_shape& v);

    static block_shape opaque;
    static block_shape transparent;
    static block_shape furniture;
    static block_shape vaccum;
};

struct cube {
    double x, y, w, h;

    bool intersects(double ox, double oy, const quad& aabb);
    bool contains(double ox, double oy, const pos2d& pos);
    double clip_x(double ox, double oy, double dx, const quad& aabb);
    double clip_y(double ox, double oy, double dy, const quad& aabb);
};

struct cube_outline : std::vector<cube> {
    static cube_outline vaccum;
    static cube_outline half;
    static cube_outline solid;

    template <typename... Args>
    static cube_outline make(const Args&... args) {
        cube_outline result_;
        (result_.push_back(args), ...);
        return result_;
    }

    bool intersects(double ox, double oy, const quad& aabb);
    bool contains(double ox, double oy, const pos2d& pos);
    double clip_x(double ox, double oy, double dx, const quad& aabb);
    double clip_y(double ox, double oy, double dy, const quad& aabb);
};

struct block_behavior;

struct block_entity {
    block_behavior* block;
    pos2i pos;
    dimension* dim;
    codec_map* cdmap;

    block_entity();
    block_entity(dimension* dim, block_behavior* block, const pos2i& pos);
};

struct block_model;

struct block_behavior : group {
    // ARC_REGISTERABLE
    block_hierarchy hierarchy = block_hierarchy::low;
    block_tick_mode tick_mode = block_tick_mode::random;
    block_shape shape = block_shape::vaccum;
    block_model* model;
    double friction = 0.65;

    bool contains(const location& loc) const override;

    std::function<float(dimension* dim, const pos2i& pos, int pipe)> cast_light;
    std::function<float(dimension* dim, const pos2i& pos, int pipe, float val)> block_light;
    std::function<quad(dimension* dim, const pos2i& pos)> render_place;
    std::function<cube_outline&(dimension* dim, const pos2i& pos, obs<entity> e)> voxel_shape;
    std::function<std::vector<item_stack>(dimension* dim, const pos2i& pos, const uuid& e)> get_loot;
    std::function<void(dimension* dim, const pos2i& pos)> tick;
    std::function<block_entity(dimension* dim, const pos2i& pos)> make_entity;
};

}  // namespace arc