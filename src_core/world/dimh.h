#pragma once
#include <vector>

#include "entity.h"
#include "world/dim.h"
#include "world/pos.h"

namespace arc {

namespace dimh {

std::vector<pos2i> get_intersected_poses(const quad& box);
std::vector<entity> get_intersected_entities(dimension* dim, const quad& box);

}  // namespace dimh

}  // namespace arc