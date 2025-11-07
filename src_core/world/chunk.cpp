#include "world/chunk.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "block.h"
#include "core/buffer.h"
#include "core/codec.h"
#include "ctt.h"
#include "entity.h"
#include "render/chunk_model.h"
#include "world/block.h"
#include "world/pos.h"

namespace arc {

chunk::~chunk() { delete model; }

void chunk::init(dimension* dim, const pos2i& pos) {
    this->dim = dim;
    this->pos = pos;
    place_cdmap_map.reserve(ARC_CHUNK_SIZE * ARC_CHUNK_SIZE);
    block_entity_map.reserve(ARC_CHUNK_SIZE * ARC_CHUNK_SIZE);
    model = new chunk_model();
    model->init(this);
}

void chunk::tick() {
    model->tick();
    tick_entities();
}

block_behavior* chunk::find_block(const pos2i& pos) {
    uint32_t bid = advance_read_ptr_<uint32_t>(blocks_.find(pos.x, pos.y));
    return R_blocks()[bid];
}

void chunk::set_block(block_behavior* block, const pos2i& pos, set_block_flag flag) {
    uint32_t bid = block->id;
    advance_write_ptr_<uint32_t>(blocks_.find(pos.x, pos.y), bid);

    if (block->shape == block_shape::furniture) {
        model->rebuild(pos, chunk_mesh_layer::furniture);
    } else {
        model->rebuild(pos, chunk_mesh_layer::block);
        model->rebuild(pos, chunk_mesh_layer::back_block);
    }
}

block_behavior* chunk::find_back_block(const pos2i& pos) {
    uint32_t bid = advance_read_ptr_<uint32_t>(back_blocks_.find(pos.x, pos.y));
    return R_blocks()[bid];
}

void chunk::set_back_block(block_behavior* block, const pos2i& pos, set_block_flag flag) {
    uint32_t bid = block->id;
    advance_write_ptr_<uint32_t>(back_blocks_.find(pos.x, pos.y), bid);

    model->rebuild(pos, chunk_mesh_layer::back_block);
    model->ao_rebuild = true;
}

obs<block_entity> chunk::find_block_entity(const pos2i& pos) {
    auto it = block_entity_map.find(pos);
    return it == block_entity_map.end() ? nullptr : it->second;
}

void chunk::set_block_entity(std::shared_ptr<block_entity> ent, const pos2i& pos) { block_entity_map[pos] = ent; }

liquid_stack chunk::find_liquid_stack(const pos2i& pos) {
    auto* ptr = liquids_.find(pos.x, pos.y);
    uint32_t lid = advance_read_ptr_<uint32_t>(ptr);
    uint8_t amt = advance_read_ptr_<uint8_t>(ptr);
    return {R_liquids()[lid], amt};
}

void chunk::set_liquid_stack(const liquid_stack& s, const pos2i& pos) {
    auto* ptr = liquids_.find(pos.x, pos.y);
    advance_write_ptr_<uint32_t>(ptr, s.liquid->id);
    advance_write_ptr_<uint8_t>(ptr, s.amount);
}

obs<codec_map> chunk::find_place_cdmap(const pos2i& pos) {
    auto it = place_cdmap_map.find(pos);
    return it == place_cdmap_map.end() ? nullptr : it->second;
}

obs<codec_map> chunk::ensure_place_cdmap(const pos2i& pos) {
    auto it = place_cdmap_map.find(pos);
    if (it == place_cdmap_map.end()) {
        place_cdmap_map[pos] = std::make_shared<codec_map>();
        return place_cdmap_map[pos];
    }
    return it->second;
}

void chunk::tick_entities() {
    for (int i = static_cast<int>(entities.size()) - 1; i >= 0; i--) {
        if (i >= static_cast<int>(entities.size())) break;

        obs<entity> e = entities[i];

        if (!e) continue;
        if (dim->ticks == e->wt_anc_) continue;  // tick wrongly invoked. When transferring this happens.
        e->wt_anc_ = dim->ticks;
        e->motion();
        if(e->tick) e->tick(e);

        if (e->parent == nullptr) e->parent = obs<chunk>::unsafe_make(this);

        const pos2i& oldpos = e->parent->pos;
        const pos2i& newpos = e->pos.findc();
        if (oldpos != newpos) {
            chunk* newc = dim->find_chunk(newpos);
            if (newc != nullptr) {
                move_entity(e.lock(), newc);
            }
        }

        if (e->is_dead) {
            remove_entity(e, true);
        }
    }
}

void chunk::remove_entity(obs<entity> e, bool global) {
    entities.erase(std::find(entities.begin(), entities.end(), e.lock()));
    e->parent = nullptr;
    // remote entities should wait for packets to be removed.
    if (global && dim->server) dim->remove_entity(e->uuid);
}

void chunk::spawn_entity(std::shared_ptr<entity> e) {
    entities.push_back(e);
    e->parent = obs<chunk>::unsafe_make(this);
}

void chunk::move_entity(std::shared_ptr<entity> e, chunk* newc) {
    remove_entity(e, false);
    newc->spawn_entity(e);
}

void chunk::clear_entity() {
    for (auto& e : entities) dim->remove_entity(e->uuid);
    entities.clear();
}

}  // namespace arc