#include "render/liquid_model.h"

#include "chunk_model.h"
#include "core/math.h"
#include "core/time.h"
#include "render/chunk_model.h"
#include "world/block.h"
#include "world/liquid.h"

namespace arc {

const int LP = 8;
const double LP_D = 8.0;

void liquid_model::default_make_liquid_(brush* brush, liquid_model* self, dimension* dim, const liquid_stack& qstack,
                                        const pos2d& pos) {
    if (qstack.is_empty()) return;

    int x = pos.x;
    int y = pos.y;

    block_behavior* bu = fast_get_block(x, y - 1);
    liquid_stack qstacku = fast_get_liquid_stack(x, y - 1);

    double p = qstack.percentage();
    double s = clock::now().seconds;

    if (qstacku.is_empty() && (p < 1.0 || !bu->shape.solid)) p += 0.025 * std::sin(x * 0.5 + y * 0.05 + s * 3.0);

    double h = p;
    double u = s * 2.0 * self->flow_speed;
    double v = s * 0.5 * self->flow_speed;

    brush->draw_texture(self->tex, quad(x, y + (1.0 - h), 1.0, h),
                        quad(u + x * LP_D, v + y * LP_D + LP_D * (1.0 - h), LP_D, LP_D * h));
    if ((!bu->shape.solid || p < 1.0) && qstacku.is_empty())
        brush->draw_texture(self->tex_edge, quad(x, y + (1.0 - h), 1.0, 1.0 / LP_D), quad(u, 0, LP_D, 1.0));
}

}  // namespace arc