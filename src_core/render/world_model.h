#pragma once

#include "gfx/brush.h"
#include "world/dim.h"

#define ARC_RENDER_ENTITY_FIND 8

namespace arc {

namespace wrd {

void retry();
void render(brush* brush, dimension* dim, const quad& cam);
void submit(brush* brush, dimension* dim);

}  // namespace wrd

}  // namespace arc