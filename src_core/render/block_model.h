#pragma once
#include <cstdint>
#include <memory>

#include "gfx/brush.h"
#include "gfx/image.h"
#include "world/block.h"

namespace arc {

enum class block_dropper : uint8_t { single, repeat, random };

struct block_model {
    ARC_REGISTERABLE
    void (*make_item)(brush* brush, block_model* self, dimension* dim, block_behavior* block,
                      const pos2d& pos) = default_make_item_;
    void (*make_block)(brush* brush, block_model* self, dimension* dim, block_behavior* block,
                       const pos2d& pos) = default_make_block_;
    void (*make_border)(brush* brush, block_model* self, dimension* dim, block_behavior* block,
                        const pos2d& pos) = default_make_border_;
    void (*make_border_back)(brush* brush, block_model* self, dimension* dim, block_behavior* block,
                        const pos2d& pos) = default_make_border_back_;
    block_dropper dropper = block_dropper::single;
    bool dynamic_render = false;
    std::shared_ptr<texture> tex = nullptr;

    static void default_make_item_(brush* brush, block_model* self, dimension* dim, block_behavior* block,
                                   const pos2d& pos);
    static void default_make_block_(brush* brush, block_model* self, dimension* dim, block_behavior* block,
                                    const pos2d& pos);
    static void default_make_border_(brush* brush, block_model* self, dimension* dim, block_behavior* block,
                                     const pos2d& pos);
    static void default_make_border_back_(brush* brush, block_model* self, dimension* dim, block_behavior* block,
                                     const pos2d& pos);
};

}  // namespace arc