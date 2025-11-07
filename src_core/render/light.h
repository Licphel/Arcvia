#pragma once

#include <atomic>
#include <memory>

#include "core/math.h"

namespace arc {

struct light_engine;
struct color;

struct ldata_ {
    static ldata_ empty;

    light_engine* engine;
    float* data;

    void ao_block(int x, int y);
    void lit_block(color color[], int x, int y, bool isWall);
};

struct dimension;
struct chunk;
struct mesh;
struct framebuffer;

struct light_engine {
    inline static constexpr float amp = 1.25;
    inline static constexpr float dark_luminance = 0.05;
    static constexpr float ult_max = 1.0;
    static constexpr float unit = ult_max / 8.0;
    static constexpr float max_v = 255;
    static constexpr float min_v = 0;
    inline static constexpr int sx = 169, sy = 144;

    float sunlight[3] = {1.0, 1.0, 1.0};
    std::shared_ptr<mesh> front_map;
    std::shared_ptr<mesh> back_map;
    std::shared_ptr<mesh> front_map_used;
    std::shared_ptr<mesh> back_map_used;
    int lorix = 0;
    int loriy = 0;
    float* lm = nullptr;
    float* lm_stable = nullptr;
    dimension* dim = nullptr;
    std::atomic_bool end_lit = false;
    std::atomic_bool start_lit = false;

    void init(dimension* dim);
    color color_stably(float x, float y);
    void tick(const quad& cam);
    void calculate(const quad& cam);
    void lit_smooth(float x, float y, float v1, float v2, float v3);
    void lit(int x, int y, float v1, float v2, float v3);
    ldata_ at(int x, int y);
    ldata_ at_stably(int x, int y);
    float get_block_shed(chunk* chunk, int x, int y, int pipe);
    float get_sky_shed(chunk* chunk, int x, int y, int pipe);
    void spread(int x, int y);
    void render_meshes(const quad& cam);
};

}  // namespace arc