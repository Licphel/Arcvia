#pragma once
#include "core/codec.h"
#include "world/group.h"

namespace arc {

struct item_behavior : group {
    //ARC_REGISTERABLE
    int max_stackable;

    bool contains(const location& loc) const override;
};

struct item_stack {
    item_behavior* item;
    std::optional<codec_map> cdmap;
    int count;

    bool is(item_behavior* item_);

    static void encode(const item_stack& stack, codec_map* cdmap);
    static void decode(item_stack& stack, codec_map* cdmap);
};

}  // namespace arc