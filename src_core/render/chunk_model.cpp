#include "render/chunk_model.h"

#include <algorithm>
#include <vector>

#include "chunk_model.h"
#include "core/log.h"
#include "core/thrp.h"
#include "ctt.h"
#include "render/block_model.h"
#include "render/chunk_model.h"
#include "world/block.h"
#include "world/chunk.h"
#include "world/dim.h"
#include "world/pos.h"

namespace arc {

static bool cmp_sorted_draw_(const chunk_model::sorted_draw_& d1, const chunk_model::sorted_draw_& d2) {
    if (d1.obj != d2.obj) return d1.obj < d2.obj;
    return d1.pos < d2.pos;
}

void chunk_model::init(chunk* chunk_) {
    parent = chunk_;
    for (int i = 0; i < ARC_CHUNK_MESH_LAYER_COUNT; i++) {
        meshes[i] = mesh::make();
        meshes_used[i] = mesh::make();
    }
}

void chunk_model::instant_rebuild(int layer) {
    int x0 = parent->pos.x * ARC_CHUNK_SIZE;
    int y0 = parent->pos.y * ARC_CHUNK_SIZE;
    std::vector<sorted_draw_> borders;
    brush* brush_ = nullptr;

    switch (static_cast<chunk_mesh_layer>(layer)) {
        case chunk_mesh_layer::back_block:
            // back block mesh
            unmeshed_back_blocks.clear();
            brush_ = meshes[layer]->retry();
            for (int x = x0; x < ARC_CHUNK_SIZE + x0; x++) {
                for (int y = y0; y < ARC_CHUNK_SIZE + y0; y++) {
                    pos2i pos = pos2i(x, y);
                    block_behavior* block = parent->find_back_block(pos);
                    if (block == block_void) continue;

                    bool rself = parent->find_block(pos)->shape != block_shape::opaque;
                    if (block->model->dynamic_render && rself) {
                        unmeshed_back_blocks.emplace_back(pos, block);
                    } else {
                        if (rself) block->model->make_block(brush_, block->model, parent->dim, block, pos.raw_2d());
                        borders.emplace_back(pos, block);
                    }
                }
            }
            std::sort(unmeshed_back_blocks.begin(), unmeshed_back_blocks.end(), cmp_sorted_draw_);
            meshes[layer]->record();

            // back block border mesh
            std::sort(borders.begin(), borders.end(), cmp_sorted_draw_);
            brush_ = meshes[layer + 1]->retry();
            for (auto& d : borders) {
                d.obj->model->make_border_back(brush_, d.obj->model, parent->dim, d.obj, d.pos.raw_2d());
            }
            meshes[layer + 1]->record();

            // push to front buffer
            unmeshed_back_blocks_used = unmeshed_back_blocks;
            std::swap(meshes[layer], meshes_used[layer]);
            std::swap(meshes[layer + 1], meshes_used[layer + 1]);
            built[layer] = true;
            built[layer + 1] = true;
            break;
        case arc::chunk_mesh_layer::furniture:
            // furniture mesh
            unmeshed_furnitures.clear();
            brush_ = meshes[layer]->retry();
            for (int x = x0; x < ARC_CHUNK_SIZE + x0; x++) {
                for (int y = y0; y < ARC_CHUNK_SIZE + y0; y++) {
                    pos2i pos = pos2i(x, y);
                    block_behavior* block = parent->find_block(pos);
                    if (block == block_void) continue;

                    if (block->model->dynamic_render) {
                        unmeshed_furnitures.emplace_back(pos, block);
                    } else {
                        block->model->make_block(brush_, block->model, parent->dim, block, pos.raw_2d());
                    }
                }
            }
            std::sort(unmeshed_furnitures.begin(), unmeshed_furnitures.end(), cmp_sorted_draw_);
            meshes[layer]->record();

            // push to front buffer
            unmeshed_furnitures_used = unmeshed_furnitures;
            std::swap(meshes[layer], meshes_used[layer]);
            built[layer] = true;
            break;
        case chunk_mesh_layer::block:
            // block mesh
            unmeshed_blocks.clear();
            brush_ = meshes[layer]->retry();
            for (int x = x0; x < ARC_CHUNK_SIZE + x0; x++) {
                for (int y = y0; y < ARC_CHUNK_SIZE + y0; y++) {
                    pos2i pos = pos2i(x, y);
                    block_behavior* block = parent->find_block(pos);
                    if (block == block_void) continue;

                    if (block->model->dynamic_render) {
                        unmeshed_blocks.emplace_back(pos, block);
                    } else {
                        block->model->make_block(brush_, block->model, parent->dim, block, pos.raw_2d());
                        borders.emplace_back(pos, block);
                    }
                }
            }
            std::sort(unmeshed_blocks.begin(), unmeshed_blocks.end(), cmp_sorted_draw_);
            meshes[layer]->record();

            // block border mesh
            std::sort(borders.begin(), borders.end(), cmp_sorted_draw_);
            brush_ = meshes[layer + 1]->retry();
            for (auto& d : borders) {
                d.obj->model->make_border(brush_, d.obj->model, parent->dim, d.obj, d.pos.raw_2d());
            }
            meshes[layer + 1]->record();

            // push to front buffer
            unmeshed_blocks_used = unmeshed_blocks;
            std::swap(meshes[layer], meshes_used[layer]);
            std::swap(meshes[layer + 1], meshes_used[layer + 1]);
            built[layer] = true;
            built[layer + 1] = true;
            break;
        default:
            break;
    }
}

void chunk_model::tick() {
    for (int i = 0; i < ARC_CHUNK_MESH_LAYER_COUNT; i++) {
        bool& mk = should_rebuild[i];
        if (mk)
#ifdef ARC_MULTITHREADED_MESH_BUILD
            thread_pool::execute([this, i]() { instant_rebuild(i); });
#else
            instant_rebuild(i);
#endif
        mk = false;
    }
}

static void try_rebuild_at(int l, int x, int y) {
    auto ptr = fast_get_chunk(x, y);
    if (ptr != nullptr) ptr->model->should_rebuild[l] = true;
}

void chunk_model::rebuild(const pos2i& pos, chunk_mesh_layer layer) {
    int l = static_cast<int>(layer);
    should_rebuild[l] = true;
    int x = pos.x;
    int y = pos.y;
    bool d10 = (x & (ARC_CHUNK_SIZE - 1)) == 0;
    bool d11 = (x & (ARC_CHUNK_SIZE - 1)) == ARC_CHUNK_SIZE - 1;
    bool d20 = (y & (ARC_CHUNK_SIZE - 1)) == 0;
    bool d21 = (y & (ARC_CHUNK_SIZE - 1)) == ARC_CHUNK_SIZE - 1;
    if (d10) try_rebuild_at(l, x - 1, y);
    if (d11) try_rebuild_at(l, x + 1, y);
    if (d20) try_rebuild_at(l, x, y - 1);
    if (d21) try_rebuild_at(l, x, y + 1);
    if (d10 && d20) try_rebuild_at(l, x - 1, y - 1);
    if (d11 && d20) try_rebuild_at(l, x + 1, y - 1);
    if (d10 && d21) try_rebuild_at(l, x - 1, y + 1);
    if (d11 && d21) try_rebuild_at(l, x + 1, y + 1);
}

void chunk_model::render(brush* brush, chunk_mesh_layer layer) {
    int l = static_cast<int>(layer);
    if (!built[l]) return;
    meshes_used[l]->draw(brush);
    switch (layer) {
        case chunk_mesh_layer::back_block:
            for (auto& d : unmeshed_back_blocks_used)
                d.obj->model->make_block(brush, d.obj->model, parent->dim, d.obj, d.pos.raw_2d());
            break;
        case chunk_mesh_layer::back_block_border:
            for (auto& d : unmeshed_back_blocks_used)
                d.obj->model->make_border_back(brush, d.obj->model, parent->dim, d.obj, d.pos.raw_2d());
            break;
        case chunk_mesh_layer::furniture:
            for (auto& d : unmeshed_furnitures_used)
                d.obj->model->make_block(brush, d.obj->model, parent->dim, d.obj, d.pos.raw_2d());
            break;
        case chunk_mesh_layer::block:
            for (auto& d : unmeshed_blocks_used)
                d.obj->model->make_block(brush, d.obj->model, parent->dim, d.obj, d.pos.raw_2d());
            break;
        case chunk_mesh_layer::block_border:
            for (auto& d : unmeshed_blocks_used)
                d.obj->model->make_border(brush, d.obj->model, parent->dim, d.obj, d.pos.raw_2d());
            break;
        default:
            break;
    }
}

static chunk* ccache_[ARC_CHUNK_CACHE_SIZE_X][ARC_CHUNK_CACHE_SIZE_Y] = {nullptr};
static int corix_ = -ARC_CHUNK_CACHE_SIZE_X / 2, coriy_ = -ARC_CHUNK_CACHE_SIZE_Y / 2;

chunk* fast_get_chunk(int x, int y) {
    x = findc(x) - corix_;
    y = findc(y) - coriy_;
    if (x < 0 || x >= ARC_CHUNK_CACHE_SIZE_X || y < 0 || y >= ARC_CHUNK_CACHE_SIZE_Y) {
        print_throw(log_level::fatal, "chunk cache is not big enought");
    }
    return ccache_[x][y];
}

block_behavior* fast_get_block(int x, int y) {
    chunk* chunk_ = fast_get_chunk(x, y);
    return chunk_ == nullptr ? block_void : chunk_->find_block({x, y});
}

block_behavior* fast_get_back_block(int x, int y) {
    chunk* chunk_ = fast_get_chunk(x, y);
    return chunk_ == nullptr ? block_void : chunk_->find_back_block({x, y});
}

void reflush_chunk_models_(dimension* dim, const pos2i& ct) {
    int x = ct.x;
    int y = ct.y;
    corix_ = x - ARC_CHUNK_CACHE_SIZE_X / 2;
    coriy_ = y - ARC_CHUNK_CACHE_SIZE_Y / 2;

    for (int i = 0; i < ARC_CHUNK_CACHE_SIZE_X; i++) {
        for (int j = 0; j < ARC_CHUNK_CACHE_SIZE_Y; j++) {
            pos2i cpos = pos2i(corix_ + i, coriy_ + j);
            auto chunk_ = dim->find_chunk(cpos);
            if (chunk_)
                ccache_[i][j] = chunk_;
            else
                ccache_[i][j] = nullptr;
        }
    }
}

}  // namespace arc