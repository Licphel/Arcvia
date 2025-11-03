#pragma once
#include "gfx/image.h"
#include "gfx/shader.h"

namespace arc {

enum class graph_mode { colored_point, colored_line, colored_triangle, colored_quad, textured_quad };

struct brush_flag {
    constexpr static long NO = 1L << 0;
    constexpr static long FLIP_X = 1L << 1;
    constexpr static long FLIP_Y = 1L << 2;
};

enum class blend_mode { normal, add };

struct graph_state {
    graph_mode mode = graph_mode::textured_quad;
    std::shared_ptr<texture> texture = nullptr;
    std::shared_ptr<program> prog = nullptr;
    std::function<void(program* program)> callback_uniform;
};

}  // namespace arc