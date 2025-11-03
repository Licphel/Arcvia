#pragma once

#include <unordered_set>

#include "core/codec.h"
#include "core/loc.h"
#include "core/reg.h"

namespace arc {

struct group {
    ARC_REGISTERABLE
    virtual ~group() = default;
    virtual bool contains(const location& loc) const = 0;

    template <typename T>
    bool contains(T* t) const {
        return contains(t->loc);
    }
};

struct group_flat : group {
    std::unordered_set<location> refs;
    std::unordered_set<location> vals;

    bool contains(const location& loc) const override;
};

struct group_map : group {
    std::unordered_set<location> refs;
    codec_map cdmap;

    bool contains(const location& loc) const override;

    template <typename T>
    T get_mapped_value(const location& loc, const T& def) const {
        if (cdmap.has(loc.concat)) return cdmap.get<T>(loc.concat, def);
        for (auto& r : refs) {
            auto* p = find_map_(r);
            if (p != nullptr && p->contains(loc)) return true;
        }
    }

   private:
    group_map* find_map_(const location& loc) const;
};

}  // namespace arc