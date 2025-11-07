#include "world/dimh.h"

#include "world/entity.h"
#include "world/pos.h"

namespace arc {

namespace dim_util {

std::vector<pos2i>& get_maybe_intersected_poses(const quad& box, double dx, double dy) {
    static thread_local std::vector<pos2i> result_;
    result_.clear();
    double i0 = dx < 0 ? dx - 1 : -1;
    double i1 = dx > 0 ? dx + 1 : 1;
    double j0 = dy < 0 ? dy - 1 : -1;
    double j1 = dy > 0 ? dy + 1 : 1;

    for (double i = box.x + i0; i < box.prom_x() + i1; i++) {
        int x = std::floor(i);
        for (double j = box.y + j0; j < box.prom_y() + j1; j++) {
            int y = std::floor(j);
            result_.push_back({x, y});
        }
    }
    return result_;
}

std::vector<obs<entity>>& get_intersected_entities(dimension* dim, const quad& box) {
    return get_intersected_entities(dim, box, [](const obs<entity>) { return true; });
}

}  // namespace dim_util

}  // namespace arc