#include "world/entity.h"

#include <cstdlib>

#include "block.h"
#include "core/math.h"
#include "core/obsptr.h"
#include "core/time.h"
#include "world/dim.h"
#include "world/dimh.h"
#include "world/liquid.h"

namespace arc {

void physic_obj::impulse(const vec2& imp) { velocity = velocity + imp / mass; }

void physic_obj::impulse_limited(const vec2& imp) {
    double dvx = imp.x / mass;
    double dvy = imp.y / mass;
    double end_x, end_y;

    if (velocity.x > ARC_MTOL)
        end_x = std::clamp(velocity.x + dvx, 0.0, 32767.0);
    else if (velocity.x < -ARC_MTOL)
        end_x = std::clamp(velocity.x + dvx, -32767.0, 0.0);

    if (velocity.y > ARC_MTOL)
        end_y = std::clamp(velocity.y + dvy, 0.0, 32767.0);
    else if (velocity.y < -ARC_MTOL)
        end_y = std::clamp(velocity.y + dvy, -32767.0, 0.0);

    velocity = vec2(end_x, end_y);
}

void physic_obj::move(const vec2& acc, const vec2& lim) {
    if (acc.x > 0 && velocity.x < lim.x)
        velocity.x = std::clamp(acc.x + velocity.x, -32767.0, lim.x);
    else if (acc.x < 0 && velocity.x > -lim.x)
        velocity.x = std::clamp(acc.x + velocity.x, -lim.x, 32767.0);

    if (acc.y > 0 && velocity.y < lim.y)
        velocity.y = std::clamp(acc.y + velocity.y, -32767.0, lim.y);
    else if (acc.y < 0 && velocity.y > -lim.y)
        velocity.y = std::clamp(acc.y + velocity.y, -lim.y, 32767.0);

    just_set_velo_ = true;
}

void physic_obj::locate(const pos2d& pos_) {
    prev_pos = pos;
    box.locate_center(pos_.x, pos_.y);
    pos = pos_;
}

void physic_obj::locate_primary(const pos2d& pos_) {
    box.locate_center(pos_.x, pos_.y);
    pos = prev_pos = pos_;
}

quad physic_obj::lerped_box_() {
    double lx = lerp(prev_pos.x, pos.x);
    double ly = lerp(prev_pos.y, pos.y);
    return quad::center(lx, ly, box.width, box.height);
}

void entity::motion() {
    double dt = clock::now().delta;
    quad origin = box;
    quad destination = box;

    // from m/s to block/s
    double dx = unit::to_block(velocity.x) * dt;
    double dy = unit::to_block(velocity.y) * dt;
    double dx0 = dx;
    double dy0 = dy;

    const auto& cols = dim_util::get_maybe_intersected_poses(box, dx, dy);

    bool stepped = false;

    if (std::abs(dx) < ARC_MTOL) dx = 0;
    if (std::abs(dy) < ARC_MTOL) dy = 0;

    // move session

    // itr block macro
#define ARC_GET_VCUBE_                                                          \
    block_behavior* block = dim->find_block(bpos);                              \
    cube_outline* cubic = nullptr;                                              \
    if (block->voxel_shape)                                                     \
        cubic = &block->voxel_shape(dim, bpos, obs<entity>::unsafe_make(this)); \
    else                                                                        \
        cubic = block->shape.solid ? &cube_outline::solid : &cube_outline::vaccum;
    // end

    collision.clear();

    if (dx != 0) {
        for (auto& bpos : cols) {
            ARC_GET_VCUBE_
            double dx0 = dx;
            dx = cubic->clip_x(bpos.x, bpos.y, dx, destination);
            if (std::abs(dx - dx0) > ARC_MTOL) collision.push_back(bpos);
        }

        // step check.
        // It's rational - we only check it when we find something blocking entity's pace forwards.
        if (cat == entity_cat::creature && (phy_status & phybit::touch_d) && std::abs(dx - dx0) > ARC_MTOL) {
            quad step_to = origin;
            float dx1 = dx0;
            step_to.translate(0, -ARC_STEP_H);

            for (auto& bpos : cols) {
                ARC_GET_VCUBE_
                dx1 = cubic->clip_x(bpos.x, bpos.y, dx1, step_to);
            }

            if (std::abs(dx1 - dx) > ARC_MTOL && std::abs(dx1) > ARC_MTOL) {
                destination.translate(dx1, -ARC_STEP_H);
                // falling check later in y axis moving.
                dy = ARC_STEP_H;
                dx = dx0;
                stepped = true;
            }

            // in this case we ignore collisions on x axis.
            collision.clear();
        }
    }

    if (!stepped) destination.translate(dx, 0);

    if (dy != 0) {
        for (auto& bpos : cols) {
            ARC_GET_VCUBE_
            double dy0 = dy;
            dy = cubic->clip_y(bpos.x, bpos.y, dy, destination);
            if (std::abs(dy - dy0) > ARC_MTOL) collision.push_back(bpos);
        }
    }

    destination.translate(0, dy);

    // post-set session
    bool xclip = std::abs(dx - dx0) > ARC_MTOL;
    bool yclip = std::abs(dy - dy0) > ARC_MTOL;

    // phy status set
    phy_status = 0;
    if (wetted_volume.size() > 0) phy_status |= phybit::swim;
    bool l, r, u, d, h, v, y;
    if ((l = dx0 < 0 && xclip)) phy_status |= phybit::touch_l;
    if ((r = dx0 > 0 && xclip)) phy_status |= phybit::touch_r;
    if ((u = dy0 < 0 && yclip)) phy_status |= phybit::touch_u;
    if ((d = dy0 > 0 && yclip)) phy_status |= phybit::touch_d;
    if ((h = r || l)) phy_status |= phybit::touch_h;
    if ((v = u || d)) phy_status |= phybit::touch_v;
    if ((y = h || v)) phy_status |= phybit::touch_y;

    if (is_spike && y) {
        dx = dy = 0;
        destination = origin;
    }

    if (dx != 0 || dy != 0) {
        net_dirty = true;
    }

    locate(pos2d(destination.center_x(), destination.center_y()));

    rotation = std::atan2(dy, dx);
    face_rot = std::atan2(face.y, face.x);

    // rebounding session
    if (d && velocity.y > 0) velocity.y *= -rebound;
    if (u && velocity.y < 0) velocity.y *= -rebound;
    // here we specially treat stepping - do not let a creature step up and stop on x axis.
    if (l && velocity.x < 0) velocity.x *= -rebound;
    if (r && velocity.x > 0) velocity.x *= -rebound;

    // ------ force set session ------ //

    // gravity session
    impulse(vec2(0, ARC_GRAVITY_A * mass));

    // liquid session
    // from block^2 to m^2
    double area_of_all = unit::to_meter2(box.area());

    if (!low_phy_sim) {
        // liquid forces calculation
        if (!just_set_velo_) {
            for (auto& kv : wetted_volume) {
                liquid_behavior* liquid = kv.first;
                double area = kv.second;
                liquid->entity_collide(obs<entity>::unsafe_make(this));
                // floating force
                impulse(vec2(0, -ARC_GRAVITY_A * liquid->density * (area / area_of_all) * dt));
                // dragging force
                double spd2 = velocity.length_powered();
                if (spd2 > ARC_MTOL) {
                    impulse(-velocity.normal() * liquid->stickiness * ARC_E_C_D * area * spd2 * 0.5 * dt);
                }
            }
        }

        // liquid collision calculation
        wetted_volume.clear();
        for (auto& pos : cols) {
            liquid_stack qstack = dim->find_liquid_stack(pos);
            if (qstack.is_empty() || wetted_volume.contains(qstack.liquid)) continue;
            quad posr = quad::corner(pos.x, pos.y, 1, qstack.percentage());
            quad inters = quad::intersection_of(posr, box);
            double area_in = unit::to_meter2(inters.area());
            if (area_in > 0) wetted_volume[qstack.liquid] += area_in;
        }
    }

    // air session
    double spd2 = velocity.length_powered();
    if (spd2 > ARC_MTOL && !low_phy_sim && !just_set_velo_) {
        impulse(-velocity.normal() * ARC_AIR_RHO * ARC_E_C_D * area_of_all * spd2 * 0.5 * dt);
    }

    // block interop session
    if (spd2 > ARC_MTOL && !just_set_velo_) {
        for (auto& bpos : collision) {
            double mu = dim->find_block(bpos)->friction;
            impulse_limited(-velocity * mu * mass * ARC_GRAVITY_A * dt);
        }
    }

    // ------ force set end ------ //

    just_set_velo_ = false;
}

}  // namespace arc