#include "world/item.h"

#include "core/codec.h"

namespace arc {

bool item_behavior::contains(const location& loc) const {
    return loc == this->loc;
}

bool item_stack::is(item_behavior* item_) {
    return item_ == item;
}

void item_stack::encode(const item_stack& stack, codec_map* cdmap) {
    cdmap->set<int>("type", stack.item->id);
    cdmap->set<int>("count", stack.count);
    if (stack.cdmap.has_value() && stack.cdmap->size() > 0) cdmap->set<codec_map>("cdmap", stack.cdmap.value());
}

void item_stack::decode(item_stack& stack, codec_map* cdmap) {
    stack.item = {};  // todo add reg
    stack.count = cdmap->get<int>("count");
    if (cdmap->has("cdmap")) stack.cdmap = cdmap->get<codec_map>("cdmap");
}

}  // namespace arc