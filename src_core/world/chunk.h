#pragma once
#include <cstdint>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <vector>

#include "core/codec.h"
#include "core/obsptr.h"
#include "world/block.h"
#include "world/liquid.h"
#include "world/pos.h"

namespace arc {

template <int C>
struct chunk_storage_ {
    uint8_t* bytes = new uint8_t[ARC_CHUNK_SIZE * ARC_CHUNK_SIZE * C]();

    ~chunk_storage_() { delete[] bytes; }

    uint8_t* find(int x, int y) { return bytes + (y * ARC_CHUNK_SIZE + x) * C; }
};

enum class set_block_flag { no = 1 << 0L, silent = 1 << 1L, admin = 1 << 2L };

struct chunk_model;
struct entity;

struct chunk : public std::enable_shared_from_this<chunk> {
    chunk_storage_<4> back_blocks_;
    chunk_storage_<4 + 2> blocks_;
    chunk_storage_<4> biomes_;
    chunk_storage_<4 + 1> liquids_;

    std::vector<std::shared_ptr<entity>> entities;
    std::unordered_map<pos2i, std::shared_ptr<block_entity>> block_entity_map;
    std::unordered_map<pos2i, std::shared_ptr<codec_map>> place_cdmap_map;
    pos2i pos;
    dimension* dim = nullptr;
    chunk_model* model = nullptr;

    ~chunk();

    void init(dimension* dim, const pos2i& pos);
    void tick();
    block_behavior* find_block(const pos2i& pos);
    void set_block(block_behavior* block, const pos2i& pos, set_block_flag flag = set_block_flag::no);
    block_behavior* find_back_block(const pos2i& pos);
    void set_back_block(block_behavior* block, const pos2i& pos, set_block_flag flag = set_block_flag::no);
    obs<block_entity> find_block_entity(const pos2i& pos);
    void set_block_entity(std::shared_ptr<block_entity> ent, const pos2i& pos);
    liquid_stack find_liquid_stack(const pos2i& pos);
    void set_liquid_stack(const liquid_stack& s, const pos2i& pos);
    obs<codec_map> find_place_cdmap(const pos2i& pos);
    obs<codec_map> ensure_place_cdmap(const pos2i& pos);
    void tick_entities();
    void remove_entity(obs<entity> e, bool global);
    void spawn_entity(std::shared_ptr<entity> e);
    void move_entity(std::shared_ptr<entity> e, chunk* newc);
    void clear_entity();
};

}  // namespace arc
