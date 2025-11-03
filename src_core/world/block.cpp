#include "world/block.h"

#include "block.h"
#include "world/dim.h"

namespace arc {

bool block_shape::operator==(const block_shape& v) {
    return v.block_light == block_light && v.solid == solid;
}

bool block_shape::operator!=(const block_shape& v) {
    return v.block_light != block_light || v.solid != solid;
}

block_shape block_shape::opaque = {true, [](float v) -> float { return v * 0.98 - 0.01; }};
block_shape block_shape::transparent = {true, [](float v) -> float { return v * 0.98 - 0.02; }};
block_shape block_shape::furniture = {false, [](float v) -> float { return v * 0.97 - 0.03; }};
block_shape block_shape::vaccum = {false, [](float v) -> float { return v * 0.96 - 0.04; }};

bool cube::intersects(double ox, double oy, const quad& aabb) {
    quad q_h = {x + ox, y + oy, w, h};
    return quad::intersect(q_h, aabb);
}

bool cube::contains(double ox, double oy, const pos2d& pos) {
    return pos.x >= ox + x && pos.y >= oy + y && pos.x <= ox + x + w && pos.y <= oy + y + h;
}

double cube::clip_x(double ox, double oy, double dx, const quad& aabb) {
    double ax = ox + x;
    double ay = oy + y;
    if (aabb.prom_y() <= ay || aabb.y >= ay + h) return dx;  // No collide on y, impossible to collide.

    if (dx > 0 && aabb.prom_x() <= ax) dx = std::min(dx, ax - aabb.prom_x());
    if (dx < 0 && aabb.x >= ax + w) dx = std::max(dx, ax + w - aabb.x);

    return dx;
}

double cube::clip_y(double ox, double oy, double dy, const quad& aabb) {
    double ax = ox + x;
    double ay = oy + y;
    if (aabb.prom_x() <= ax || aabb.x >= ax + w) return dy;  // No collide on y, impossible to collide.

    if (dy > 0 && aabb.prom_y() <= ay) dy = std::min(dy, ay - aabb.prom_y());
    if (dy < 0 && aabb.y >= ay + h) dy = std::max(dy, ay + h - aabb.y);

    return dy;
}

bool cube_outline::intersects(double ox, double oy, const quad& aabb) {
    for (auto& c : *this)
        if (c.intersects(ox, oy, aabb)) return true;
    return false;
}

bool cube_outline::contains(double ox, double oy, const pos2d& pos) {
    for (auto& c : *this)
        if (c.contains(ox, oy, pos)) return true;
    return false;
}

double cube_outline::clip_x(double ox, double oy, double dx, const quad& aabb) {
    for (auto& c : *this) dx = c.clip_x(ox, oy, dx, aabb);
    return dx;
}

double cube_outline::clip_y(double ox, double oy, double dy, const quad& aabb) {
    for (auto& c : *this) dy = c.clip_y(ox, oy, dy, aabb);
    return dy;
}

block_entity::block_entity() = default;
block_entity::block_entity(dimension* dim, block_behavior* block, const pos2i& pos) : dim(dim), block(block), pos(pos) {
    cdmap = dim->ensure_place_cdmap(pos);
}

bool block_behavior::contains(const location& loc) const { return loc == this->loc; }

}  // namespace arc