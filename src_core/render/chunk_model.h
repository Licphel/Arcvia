#pragma once
#include <atomic>
#include <memory>
#include <vector>

#include "core/obsptr.h"
#include "gfx/brush.h"
#include "gfx/mesh.h"
#include "world/block.h"
#include "world/dim.h"
#include "world/pos.h"

#define ARC_CHUNK_CACHE_SIZE_X 10
#define ARC_CHUNK_CACHE_SIZE_Y 8
// #define ARC_MULTITHREADED_MESH_BUILD

namespace arc {

#define ARC_CHUNK_MESH_LAYER_COUNT 6
enum class chunk_mesh_layer {
    back_block,
    back_block_border,
    furniture,
    block,
    block_border,
    overlay,
};

struct chunk;

struct chunk_model {
    struct sorted_draw_ {
        pos2i pos;
        block_behavior* obj;
    };

    chunk* parent;
    std::atomic_bool should_rebuild[ARC_CHUNK_MESH_LAYER_COUNT] = {false};
    std::atomic_bool ao_rebuild = false;
    std::atomic_bool built[ARC_CHUNK_MESH_LAYER_COUNT] = {false};
    // double buffer
    std::shared_ptr<mesh> meshes[ARC_CHUNK_MESH_LAYER_COUNT];
    std::vector<sorted_draw_> unmeshed_back_blocks;
    std::vector<sorted_draw_> unmeshed_furnitures;
    std::vector<sorted_draw_> unmeshed_blocks;
    std::shared_ptr<mesh> meshes_used[ARC_CHUNK_MESH_LAYER_COUNT];
    std::vector<sorted_draw_> unmeshed_back_blocks_used;
    std::vector<sorted_draw_> unmeshed_furnitures_used;
    std::vector<sorted_draw_> unmeshed_blocks_used;

    void init(chunk* chunk_);
    void tick();
    void instant_rebuild(int layer);
    void rebuild(const pos2i& pos, chunk_mesh_layer layer);
    void render(brush* brush, chunk_mesh_layer layer);
};

obs<chunk> fast_get_chunk(int x, int y);
block_behavior* fast_get_block(int x, int y);
block_behavior* fast_get_back_block(int x, int y);
void reflush_chunk_models_(dimension* dim, const pos2i& ct);

}  // namespace arc