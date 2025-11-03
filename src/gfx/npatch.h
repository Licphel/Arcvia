#pragma once

#include <memory>

#include "core/math.h"

namespace arc {

struct texture;
struct brush;

struct nine_patches {
    std::shared_ptr<texture> b;
    std::shared_ptr<texture> c;
    std::shared_ptr<texture> l;
    std::shared_ptr<texture> lb;
    std::shared_ptr<texture> lt;
    std::shared_ptr<texture> r;
    std::shared_ptr<texture> rb;
    std::shared_ptr<texture> rt;
    std::shared_ptr<texture> t;
    double scale;
    double th;
    double tw;
    // enable overlapping can make the size control of nine-patches more
    // accurate. however, if your texture contains some transparent parts, it's
    // better to turn it off.
    bool enable_overlapping_ = true;

    nine_patches();
    nine_patches(std::shared_ptr<texture> tex);
    nine_patches(std::shared_ptr<texture> tex, double scale);

    void make_vtx(brush* brush, const quad& dst) const;
};

}  // namespace arc