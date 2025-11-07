#include "world/dim.h"

#include "ctt.h"
#include "entity.h"
#include "render/light.h"
#include "world/block.h"
#include "world/liquid.h"

namespace arc {

void dimension::init() {
    light_executor = std::make_unique<light_engine>();
    light_executor->init(this);
}

void dimension::tick() {
    for (auto& kv : chunk_map) {
        kv.second->tick();
    }
    ticks++;
}

obs<chunk> dimension::find_chunk(const pos2i& pos, find_chunk_flag flag) {
    auto it = chunk_map.find(pos);
    if (it != chunk_map.end()) {
        return it->second;
    }
    switch (flag) {
        case find_chunk_flag::no:
            return nullptr;
        case find_chunk_flag::cache:
            it = chunk_cache_map.find(pos);
            return it == chunk_cache_map.end() ? nullptr : it->second;
        case find_chunk_flag::mk_cache_if_absent:
            std::shared_ptr<chunk> chunk_ = std::make_shared<chunk>();
            chunk_->init(this, pos);
            chunk_cache_map[pos] = chunk_;
            return chunk_;
    }
    return nullptr;
}

obs<chunk> dimension::find_chunk_by_block(const pos2i& pos, find_chunk_flag flag) {
    return find_chunk(pos.findc(), flag);
}

std::shared_ptr<chunk> dimension::consume_chunk_cache(const pos2i& pos) {
    std::lock_guard<std::mutex> lock(chunkcop_mutex_);
    auto ckc = chunk_cache_map.find(pos);
    if (ckc == chunk_cache_map.end()) {
        return nullptr;
    }
    chunk_cache_map.erase(ckc);
    return ckc->second;
}

void dimension::set_chunk(const pos2i& pos, std::shared_ptr<chunk> chunk_) {
    std::lock_guard<std::mutex> lock(chunkop_mutex_);
    chunk_map[pos] = chunk_;
}

void dimension::set_chunk_cache(const pos2i& pos, std::shared_ptr<chunk> chunk_) {
    std::lock_guard<std::mutex> lock(chunkcop_mutex_);
    chunk_cache_map[pos] = chunk_;
}

block_behavior* dimension::find_block(const pos2i& pos) {
    auto chunk_ = find_chunk_by_block(pos, find_chunk_flag::cache);
    return chunk_ == nullptr ? block_void : chunk_->find_block(pos);
}

void dimension::set_block(block_behavior* block, const pos2i& pos, set_block_flag flag) {
    auto chunk_ = find_chunk_by_block(pos, find_chunk_flag::mk_cache_if_absent);
    chunk_->set_block(block, pos, flag);
}

block_behavior* dimension::find_back_block(const pos2i& pos) {
    auto chunk_ = find_chunk_by_block(pos, find_chunk_flag::cache);
    return chunk_ == nullptr ? block_void : chunk_->find_back_block(pos);
}

void dimension::set_back_block(block_behavior* block, const pos2i& pos, set_block_flag flag) {
    auto chunk_ = find_chunk_by_block(pos, find_chunk_flag::mk_cache_if_absent);
    chunk_->set_back_block(block, pos, flag);
}

obs<block_entity> dimension::find_block_entity(const pos2i& pos) {
    auto chunk_ = find_chunk_by_block(pos, find_chunk_flag::cache);
    return chunk_ == nullptr ? nullptr : chunk_->find_block_entity(pos);
}

void dimension::set_block_entity(std::shared_ptr<block_entity> ent, const pos2i& pos) {
    auto chunk_ = find_chunk_by_block(pos, find_chunk_flag::mk_cache_if_absent);
    chunk_->set_block_entity(ent, pos);
}

liquid_stack dimension::find_liquid_stack(const pos2i& pos) {
    auto chunk_ = find_chunk_by_block(pos, find_chunk_flag::cache);
    return chunk_ == nullptr ? liquid_stack(liquid_void, 0) : chunk_->find_liquid_stack(pos);
}

void dimension::set_liquid_stack(const liquid_stack& s, const pos2i& pos) {
    auto chunk_ = find_chunk_by_block(pos, find_chunk_flag::mk_cache_if_absent);
    chunk_->set_liquid_stack(s, pos);
}

obs<codec_map> dimension::find_place_cdmap(const pos2i& pos) {
    auto chunk_ = find_chunk_by_block(pos, find_chunk_flag::cache);
    return chunk_ == nullptr ? nullptr : chunk_->find_place_cdmap(pos);
}

obs<codec_map> dimension::ensure_place_cdmap(const pos2i& pos) {
    auto chunk_ = find_chunk_by_block(pos, find_chunk_flag::mk_cache_if_absent);
    return chunk_->ensure_place_cdmap(pos);
}

void dimension::spawn_entity(std::shared_ptr<entity> e) {
    e->dim = this;
    obs<chunk> chunk_ = find_chunk(e->pos.findc());
    e->parent = chunk_;
    if (chunk_) chunk_->spawn_entity(e);
    entities[e->uuid] = e;
}

obs<entity> dimension::find_entity(const uuid& id) { return entities[id]; }

void dimension::remove_entity(const uuid& id) {
    auto it = entities.find(id);
    if (it == entities.end()) return;

    obs<chunk> chunk_ = it->second->parent;
    if (chunk_) chunk_->remove_entity(it->second, false);
    entities.erase(it);
}

}  // namespace arc