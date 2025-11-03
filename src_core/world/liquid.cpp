#include "world/liquid.h"

#include <cstdint>


namespace arc {

uint8_t liquid_stack::max_amount = 128;
bool liquid_stack::is_empty() { return amount == 0 || liquid == nullptr; }
bool liquid_stack::is_full() { return amount == max_amount; }
bool liquid_stack::is(liquid_behavior* liquid_) { return liquid_ == liquid; }

bool liquid_behavior::contains(const location& loc) const {
    return loc == this->loc;
}

}  // namespace arc