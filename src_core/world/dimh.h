#pragma once
#include <unordered_set>
#include <vector>

#include "entity.h"
#include "world/dim.h"
#include "world/pos.h"

#define ARC_FIND_ENTITY_INFLATION 4

namespace arc {

namespace dim_util {

// thread local return value. do not use a legacy value.
std::vector<pos2i>& get_maybe_intersected_poses(const quad& box, double dx, double dy);
// thread local return value. do not use a legacy value.
std::vector<obs<entity>>& get_intersected_entities(dimension* dim, const quad& box);

// thread local return value. do not use a legacy value.
template <typename F>
std::vector<obs<entity>>& get_intersected_entities(dimension* dim, const quad& box, F&& predicate) {
    static thread_local std::vector<obs<entity>> result_;
    static thread_local std::unordered_set<uuid> result_set_;
    result_.clear();
    result_set_.clear();

    int i1 = std::floor((box.x - ARC_FIND_ENTITY_INFLATION) / 16.0);
    int i2 = std::floor((box.prom_x() + ARC_FIND_ENTITY_INFLATION) / 16.0);
    int j1 = std::floor((box.y - ARC_FIND_ENTITY_INFLATION) / 16.0);
    int j2 = std::floor((box.prom_y() + ARC_FIND_ENTITY_INFLATION) / 16.0);

    for (int i = i1; i <= i2; i++) {
        for (int j = j1; j <= j2; j++) {
            obs<chunk> chunk = dim->find_chunk({i, j});
            if (!chunk) continue;

            for (obs<entity> e : chunk->entities) {
                if (!e || e->is_dead) continue;
                if(!predicate(e)) continue;
                if (quad::intersect(e->box, box) && result_set_.find(e->uuid) == result_set_.end()) {
                    result_.push_back(e);
                    result_set_.insert(e->uuid);
                }
            }
        }
    }
    return result_;
}

}  // namespace dim_util

}  // namespace arc