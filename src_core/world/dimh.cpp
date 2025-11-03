#include "world/dimh.h"

namespace arc {

namespace dimh {

std::vector<pos2i> get_intersected_poses(const quad& box) {
    std::vector<pos2i> result_;
    for (double i = box.x - 1; i < box.prom_x() + 1; i++) {
        int x = std::floor(i);
        for (double j = box.x - 1; j < box.prom_x() + 1; j++) {
            int y = std::floor(j);
            result_.push_back({x, y});
        }
    }
    return result_;
}

std::vector<entity> get_intersected_entities(dimension* dim, const quad& box) {
    return {};
}

}  // namespace dimh

}  // namespace arc