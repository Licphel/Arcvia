#pragma once
#include "core/buffer.h"

namespace arc {

struct random {
    struct impl_;
    std::unique_ptr<impl_> pimpl_;

    random();
    ~random();

    // here, the implementation may be complicated.
    // but this single seed provides randomness.
    void set_seed(long seed);

    // equivalent to next() < 0.5.
    bool next_bool();
    double next();
    double next(double min, double max);
    // generates a guassian-distributed double in [min, max].
    double next_guassian(double min, double max);
    int next_int(int bound);
    int next_int(int min, int max);

    void write(byte_buf& buf);
    void read(byte_buf& buf);

    std::shared_ptr<random> copy();
    std::shared_ptr<random> copy(int seed_addon);

    // get a global random generator, when we don't care the seed.
    static std::shared_ptr<random> rg;
    // make a new random generator, with a random seed or a specific seed.
    static std::shared_ptr<random> make();
    static std::shared_ptr<random> make(long seed);
};

struct noise {
    long seed;
    virtual ~noise() = default;
    virtual double generate(double x, double y, double z) = 0;

    static std::shared_ptr<noise> make_perlin(long seed);
    static std::shared_ptr<noise> make_voronoi(long seed);
};

}  // namespace arc
