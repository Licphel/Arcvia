#include "render/block_model.h"

#include <memory>

#include "ctt.h"
#include "gfx/brush.h"
#include "render/chunk_model.h"
#include "world/block.h"

namespace arc {

const double OLN = 0.002;
const double OLN4 = OLN * 4;
const int BP = 8;
const double BP_D = 8.0;

constexpr static bool iscnnt_(block_behavior* r1, block_behavior* r2) { return r1 == r2; }

inline static std::shared_ptr<texture> get_block_ico_(block_behavior* r) { return R_block_models()[r->loc]->tex; }

constexpr static long place_hash(int x, int y) {
    long hash = (long)x * 374761393 + (long)y * 668265263;
    hash = (hash ^ (hash >> 15)) * 2246822519;
    hash = (hash ^ (hash >> 13)) * 3266489917;
    hash ^= (hash >> 16);
    return hash;
}

void block_model::default_make_item_(brush* brush, block_model* self, dimension* dim, block_behavior* block,
                                     const pos2d& pos) {
    std::shared_ptr<texture> tex = get_block_ico_(block);
    brush->draw_texture(tex, quad(68, 16, 16, 16));
}

void block_model::default_make_block_(brush* brush, block_model* self, dimension* dim, block_behavior* block,
                                      const pos2d& pos) {
    block_dropper dropper = self->dropper;
    std::shared_ptr<texture> tex = get_block_ico_(block);

    int x = static_cast<int>(pos.x);
    int y = static_cast<int>(pos.y);

    quad place = block->render_place ? block->render_place(dim, pos) : quad(0.0, 0.0, 1.0, 1.0);
    place.inflate(OLN4, OLN4);
    place.translate(pos.x, pos.y);

    if (dropper == block_dropper::single) {
        brush->draw_texture(tex, place);
    } else if (dropper == block_dropper::repeat) {
        double u = std::abs(x) % (tex->height / BP) * BP;
        double v = std::abs(y) % (tex->height / BP) * BP;
        quad area = quad(u, v, BP_D, BP_D);
        brush->draw_texture(tex, place, area);
    } else if (dropper == block_dropper::random) {
        long hash = place_hash(x, y);
        double u = static_cast<int>(std::abs(hash * 2 + 5) % (tex->width / BP)) * BP;
        double v = static_cast<int>(std::abs(hash * 3 - 7) % (tex->height / BP)) * BP;
        quad area = quad(u, v, BP_D, BP_D);
        brush->draw_texture(tex, place, area);
    }
}

template <int Binding_option_>
void raw_make_border_(brush* brush, block_model* self, dimension* dim, block_behavior* block, const pos2d& pos) {
    const bool RANDOM_BORDER = true;

    if (!block->shape.solid) return;

    std::shared_ptr<texture> tex = get_block_ico_(block);
    int x = pos.x;
    int y = pos.y;

    block_behavior* ub;
    block_behavior* db;
    block_behavior* lb;
    block_behavior* rb;
    block_behavior* lub;
    block_behavior* ldb;
    block_behavior* rub;
    block_behavior* rdb;

    if constexpr (Binding_option_ == 0) {
        ub = fast_get_block(x, y - 1);
        db = fast_get_block(x, y + 1);
        lb = fast_get_block(x - 1, y);
        rb = fast_get_block(x + 1, y);
        lub = fast_get_block(x - 1, y - 1);
        ldb = fast_get_block(x - 1, y + 1);
        rub = fast_get_block(x + 1, y - 1);
        rdb = fast_get_block(x + 1, y + 1);
    } else if constexpr (Binding_option_ == 1) {
        ub = fast_get_back_block(x, y - 1);
        db = fast_get_back_block(x, y + 1);
        lb = fast_get_back_block(x - 1, y);
        rb = fast_get_back_block(x + 1, y);
        lub = fast_get_back_block(x - 1, y - 1);
        ldb = fast_get_back_block(x - 1, y + 1);
        rub = fast_get_back_block(x + 1, y - 1);
        rdb = fast_get_back_block(x + 1, y + 1);
    }

    bool up = iscnnt_(ub, block);
    bool down = iscnnt_(db, block);
    bool left = iscnnt_(lb, block);
    bool right = iscnnt_(rb, block);
    bool upl = iscnnt_(lub, block);
    bool downl = iscnnt_(ldb, block);
    bool upr = iscnnt_(rub, block);
    bool downr = iscnnt_(rdb, block);

    long hash = place_hash(x, y);
    int rdu = RANDOM_BORDER ? static_cast<int>(std::abs(hash) % 4) * 13 : 0;
    int rdu2 = RANDOM_BORDER ? static_cast<int>(std::abs(hash + 1) % 4) * 9 : 0;

    int pid = block->id;

    if (!left && (lb->id <= pid || !lb->shape.solid)) {
        if (downl)
            brush->draw_texture(tex, quad(x - 0.5 - OLN, y + 0.5 - OLN, 0.5 + OLN4, 0.5 + OLN4),
                                quad(rdu2 + 37, 17, 4, 4));
        else
            brush->draw_texture(tex, quad(x - 0.25 - OLN, y + 0.5 - OLN, 0.25 + OLN4, 0.5 + OLN4),
                                quad(rdu + 33, 6, 2, 4));
        if (upl)
            brush->draw_texture(tex, quad(x - 0.5 - OLN, y + OLN, 0.5 + OLN4, 0.5 + OLN4), quad(rdu2 + 37, 13, 4, 4));
        else
            brush->draw_texture(tex, quad(x - 0.25 - OLN, y + OLN, 0.25 + OLN4, 0.5 + OLN4), quad(rdu + 33, 2, 2, 4));
    }

    if (!up && (ub->id <= pid || !ub->shape.solid)) {
        if (upl)
            brush->draw_texture(tex, quad(x - OLN, y - 0.5 - OLN, 0.5 + OLN4, 0.5 + OLN4), quad(rdu2 + 33, 17, 4, 4));
        else
            brush->draw_texture(tex, quad(x - OLN, y - 0.25 - OLN, 0.5 + OLN4, 0.25 + OLN4), quad(rdu + 35, 0, 4, 2));
        if (!upr)
            brush->draw_texture(tex, quad(x + 0.5 + OLN, y - 0.25 - OLN, 0.5 + OLN4, 0.25 + OLN4),
                                quad(rdu + 39, 0, 4, 2));
    }

    if (!right && (rb->id <= pid || !rb->shape.solid)) {
        if (!downr)
            brush->draw_texture(tex, quad(x + 1 - OLN, y + 0.5 - OLN, 0.25 + OLN4, 0.5 + OLN4),
                                quad(rdu + 43, 6, 2, 4));
        if (upr)
            brush->draw_texture(tex, quad(x + 1 - OLN, y + OLN, 0.5 + OLN4, 0.5 + OLN4), quad(rdu2 + 33, 13, 4, 4));
        else
            brush->draw_texture(tex, quad(x + 1 - OLN, y + OLN, 0.25 + OLN4, 0.5 + OLN4), quad(rdu + 43, 2, 2, 4));
    }

    if (!down && (db->id <= pid || !db->shape.solid)) {
        if (!downl)
            brush->draw_texture(tex, quad(x - OLN, y + 1.0 - OLN, 0.5 + OLN4, 0.25 + OLN4), quad(rdu + 35, 10, 4, 2));
        if (!downr)
            brush->draw_texture(tex, quad(x + 0.5 + OLN, y + 1.0 - OLN, 0.5 + OLN4, 0.25 + OLN4),
                                quad(rdu + 39, 10, 4, 2));
    }
}

void block_model::default_make_border_(brush* brush, block_model* self, dimension* dim, block_behavior* block,
                                       const pos2d& pos) {
    raw_make_border_<0>(brush, self, dim, block, pos);
}

void block_model::default_make_border_back_(brush* brush, block_model* self, dimension* dim, block_behavior* block,
                                       const pos2d& pos) {
    raw_make_border_<1>(brush, self, dim, block, pos);
}

}  // namespace arc