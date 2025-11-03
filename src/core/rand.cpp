#include "core/rand.h"

#include <random>

#include "core/buffer.h"

namespace arc {

struct random::impl_ {
    long a, b;
};

random::random() : pimpl_(std::make_unique<impl_>()) {}

random::~random() = default;

void random::set_seed(long seed) {
    pimpl_->a = (seed + 13) / 2;
    pimpl_->b = (seed - 9) ^ 39;
}

bool random::next_bool() { return next() < 0.5; }

double random::next() {
    thread_local static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng);
}

double random::next(double min, double max) { return next() * (max - min) + min; }

double random::next_guassian(double min, double max) {
    double x, y, w;
    do {
        x = next() * 2 - 1;
        y = next() * 2 - 1;
        w = x * x + y * y;
    } while (w >= 1 || w == 0);

    double c = sqrt(-2 * log(w) / w);
    return (y * c) * (max - min) + min;
}

int random::next_int(int bound) {
    thread_local static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, bound - 1);
    return dist(rng);
}

int random::next_int(int min, int max) { return next_int(max + 1 - min) + min; }

void random::write(byte_buf& buf) {
    buf.write<long>(pimpl_->a);
    buf.write<long>(pimpl_->b);
}

void random::read(byte_buf& buf) {
    pimpl_->a = buf.read<long>();
    pimpl_->b = buf.read<long>();
}

std::shared_ptr<random> random::copy() { return copy(0); }

std::shared_ptr<random> random::copy(int seed_addon) {
    auto ptr = make();
    ptr->pimpl_->a = pimpl_->a + seed_addon;
    ptr->pimpl_->b = pimpl_->b - seed_addon;
    return ptr;
}

std::shared_ptr<random> random::rg = make();

std::shared_ptr<random> random::make() {
    thread_local static std::mt19937_64 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, INT_MAX);
    return make(dist(rng));
}

std::shared_ptr<random> random::make(long seed) {
    auto ptr = std::make_shared<random>();
    ptr->set_seed(seed);
    return ptr;
}

struct noise_perlin_ : noise {
    int p[512];

    void init() {
        auto rd = random::make(seed);

        int perm[256];
        for (int i = 0; i < 256; i++) {
            perm[i] = i;
        }

        for (int i = 255; i > 0; i--) {
            int j = rd->next_int(i + 1);
            int temp = perm[i];
            perm[i] = perm[j];
            perm[j] = temp;
        }

        for (int i = 0; i < 256; i++) {
            p[i] = p[i + 256] = perm[i];
        }
    }

    inline int fast_floor(double x) const {
        int xi = static_cast<int>(x);
        return x < xi ? xi - 1 : xi;
    }

    inline double fade(double t) const { return t * t * t * (t * (t * 6 - 15) + 10); }

    inline double lerp(double t, double a, double b) const { return a + t * (b - a); }

    inline double grad(int hash, double x, double y, double z) const {
        int h = hash & 15;

        switch (h & 7) {
            case 0:
                return x + y;
            case 1:
                return -x + y;
            case 2:
                return x - y;
            case 3:
                return -x - y;
            case 4:
                return x + z;
            case 5:
                return -x + z;
            case 6:
                return x - z;
            case 7:
                return -x - z;
            default:
                return 0;
        }
    }

    double generate(double x, double y, double z) override {
        int X = fast_floor(x) & 255;
        int Y = fast_floor(y) & 255;
        int Z = fast_floor(z) & 255;

        x -= fast_floor(x);
        y -= fast_floor(y);
        z -= fast_floor(z);

        double u = fade(x);
        double v = fade(y);
        double w = fade(z);

        int A = p[X] + Y;
        int AA = p[A] + Z;
        int AB = p[A + 1] + Z;
        int B = p[X + 1] + Y;
        int BA = p[B] + Z;
        int BB = p[B + 1] + Z;

        double result = lerp(w,
                             lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
                                  lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
                             lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)),
                                  lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))));

        return (result + 1.0) * 0.5;
    }
};

struct noise_voronoi_ : noise {
    inline int floor(double v) {
        int i = static_cast<int>(v);
        return v >= i ? i : i - 1;
    }

    inline double seedl(int x, int y, int z, long seed) {
        long v1 = (x + 2687 * y + 433 * z + 941 * seed) & INT_MAX;
        long v2 = (v1 * (v1 * v1 * 113 + 653) + 2819) & INT_MAX;
        return 1 - static_cast<double>(v2) / static_cast<double>(INT_MAX);
    }

    double generate(double x, double y, double z) override {
        int x0 = floor(x);
        int y0 = floor(y);
        int z0 = floor(z);

        double xc = 0;
        double yc = 0;
        double zc = 0;
        double md = INT_MAX;

        for (int k = z0 - 2; k <= z0 + 2; k++)
            for (int j = y0 - 2; j <= y0 + 2; j++)
                for (int i = x0 - 2; i <= x0 + 2; i++) {
                    double xp = i + seedl(i, j, k, seed);
                    double yp = j + seedl(i, j, k, seed + 1);
                    double zp = k + seedl(i, j, k, seed + 2);
                    double xd = xp - x;
                    double yd = yp - y;
                    double zd = zp - z;
                    double d = xd * xd + yd * yd + zd * zd;

                    if (d < md) {
                        md = d;
                        xc = xp;
                        yc = yp;
                        zc = zp;
                    }
                }

        return seedl(floor(xc), floor(yc), floor(zc), 0);
    }
};

std::shared_ptr<noise> noise::make_perlin(long seed) {
    auto ptr = std::make_shared<noise_perlin_>();
    ptr->seed = seed;
    ptr->init();
    return ptr;
}

std::shared_ptr<noise> noise::make_voronoi(long seed) {
    auto ptr = std::make_shared<noise_voronoi_>();
    ptr->seed = seed;
    return ptr;
}

}  // namespace arc