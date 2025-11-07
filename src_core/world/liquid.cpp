#include "world/liquid.h"

#include <cstdint>

#include "block.h"
#include "core/rand.h"
#include "ctt.h"
#include "world/chunk.h"
#include "world/dim.h"
#include "world/pos.h"

namespace arc {

uint8_t liquid_stack::max_amount = 128;
bool liquid_stack::is_empty() const { return amount == 0 || liquid == liquid_void; }
bool liquid_stack::is_full() const { return amount == max_amount; }
bool liquid_stack::is(liquid_behavior* liquid_) const { return liquid_ == liquid; }
double liquid_stack::percentage() const { return static_cast<double>(amount) / max_amount; }

bool liquid_behavior::contains(const location& loc) const { return loc == this->loc; }

// liquid flowing

static bool spread_leftward = false;
static bool last_leftward = false;

static void spread_liquid_(obs<chunk>& here, obs<chunk>& l, obs<chunk>& r, obs<chunk>& d, obs<chunk>& u, int x, int y) {
    pos2i p_0 = pos2i(x, y);
    liquid_stack qstack = here->find_liquid_stack(p_0);
    if (qstack.is_empty()) return;

    liquid_behavior* type = qstack.liquid;
    int a = qstack.amount;

    int sox = wrap_pos(x);
    int soy = wrap_pos(y);

    block_behavior* bd;
    liquid_stack qstackd;
    obs<chunk> cfind_d;

    pos2i p_d = pos2i(x, y + 1);
    pos2i p_l = pos2i(x - 1, y);
    pos2i p_r = pos2i(x + 1, y);

    if (d || soy != ARC_CHUNK_SIZE - 1) {
        if (soy == ARC_CHUNK_SIZE - 1) {
            bd = d->find_block(p_d);
            cfind_d = d;
            qstackd = cfind_d->find_liquid_stack(p_d);
        } else {
            bd = here->find_block(p_d);
            cfind_d = here;
            qstackd = cfind_d->find_liquid_stack(p_d);
        }

        int ad = qstackd.amount;
        liquid_behavior* ld = qstackd.liquid;

        if (!bd->shape.solid) {
            if (ld != liquid_void && ld != type) {
                type->on_touch(here->dim, p_0, qstack, p_d, qstackd);
            } else if (ad < liquid_stack::max_amount) {
                int ext = std::min(liquid_stack::max_amount - ad, a);
                a -= ext;
                here->set_liquid_stack(liquid_stack(type, a), p_0);
                cfind_d->set_liquid_stack(liquid_stack(type, ext + ad), p_d);
                if (ext > a / 2.0) return;
            }
        }
    }

    block_behavior *blockr, *blockl;
    obs<chunk> cfind_r, cfind_l;
    int amountr, amountl;
    liquid_stack qstackr, qstackl;
    liquid_behavior *liquidr, *liquidl;

    bool can_l = true, can_r = true;

    if (sox == 0) {
        if (!l) can_l = false;
        cfind_l = l;
        cfind_r = here;
    } else if (sox == 15) {
        if (!r) can_r = false;
        cfind_l = here;
        cfind_r = r;
    } else {
        cfind_l = here;
        cfind_r = here;
    }

    if (can_r) blockr = cfind_r->find_block(p_r);
    if (can_l) blockl = cfind_l->find_block(p_l);
    if (can_r) qstackr = cfind_r->find_liquid_stack(p_r);
    if (can_l) qstackl = cfind_l->find_liquid_stack(p_l);
    amountr = qstackr.amount;
    amountl = qstackl.amount;
    liquidr = qstackr.liquid;
    liquidl = qstackl.liquid;

    if (can_l && spread_leftward && !blockl->shape.solid) {
        if (liquidl != liquid_void && liquidl != type) {
            type->on_touch(here->dim, p_0, qstack, p_l, qstackl);
        } else {
            int ext = std::min(liquid_stack::max_amount - amountl,
                               std::min(static_cast<int>(std::ceil((a - amountl) / 2.0)), a));
            a -= ext;
            here->set_liquid_stack(liquid_stack(type, a), p_0);
            cfind_l->set_liquid_stack(liquid_stack(type, ext + amountl), p_l);
        }
    } else if (can_r && !blockr->shape.solid) {
        if (liquidr != liquid_void && liquidr != type) {
            type->on_touch(here->dim, p_0, qstack, p_r, qstackr);
        } else {
            int ext = std::min(liquid_stack::max_amount - amountr,
                               std::min(static_cast<int>(std::ceil((a - amountr) / 2.0)), a));
            a -= ext;
            here->set_liquid_stack(liquid_stack(type, a), p_0);
            cfind_r->set_liquid_stack(liquid_stack(type, ext + amountr), p_r);
        }
    }
}

liquid_flow_engine::liquid_flow_engine(obs<chunk> chunk_) {
    obs<chunk> chunk0 = chunk_->dim->find_chunk(pos2i(chunk_->pos.x - 1, chunk_->pos.y));
    obs<chunk> chunk1 = chunk_->dim->find_chunk(pos2i(chunk_->pos.x + 1, chunk_->pos.y));
    obs<chunk> chunk2 = chunk_->dim->find_chunk(pos2i(chunk_->pos.x, chunk_->pos.y + 1));
    obs<chunk> chunk3 = chunk_->dim->find_chunk(pos2i(chunk_->pos.x, chunk_->pos.y - 1));

    spread_leftward = random::rg->next_bool();

    if (!last_leftward)
        for (int x = chunk_->min_x; x <= chunk_->max_x; x++)
            for (int y = chunk_->max_y; y >= chunk_->min_y; y--)
                spread_liquid_(chunk_, chunk0, chunk1, chunk2, chunk3, x, y);
    else
        for (int x = chunk_->max_x; x >= chunk_->min_x; x--)
            for (int y = chunk_->max_y; y >= chunk_->min_y; y--)
                spread_liquid_(chunk_, chunk0, chunk1, chunk2, chunk3, x, y);

    last_leftward = !last_leftward;
}

}  // namespace arc