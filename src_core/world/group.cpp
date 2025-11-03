#include "world/group.h"
#include "ctt.h"

namespace arc {

bool group_flat::contains(const location& loc) const {
    auto it = vals.find(loc);
    if (it != vals.end()) 
        return true;
    for (auto& r : refs) {
        auto* g = R_groups()[r];
        auto* p = R_group_maps()[r];
        if(g != nullptr && g->contains(loc))
            return true;
        if(p != nullptr && g->contains(loc))
            return true;
    }
    return false;
}

bool group_map::contains(const location& loc) const {
    if(cdmap.has(loc.concat))
        return true;
    for (auto& r : refs) {
        auto* g = R_groups()[r];
        auto* p = R_group_maps()[r];
        if(g != nullptr && g->contains(loc))
            return true;
        if(p != nullptr && g->contains(loc))
            return true;
    }
    return false;
}

group_map* group_map::find_map_(const location& loc) const {
    return R_group_maps()[loc];
}

}  // namespace arc