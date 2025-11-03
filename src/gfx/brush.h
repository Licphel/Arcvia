#pragma once
#include <stack>

#include "core/math.h"
#include "gfx/buffer.h"
#include "gfx/camera.h"
#include "gfx/color.h"
#include "gfx/image.h"
#include "gfx/mesh.h"
#include "gfx/shader.h"
#include "gfx/state.h"

// I've not found the solution, on how to correctly transform models on cpu.
// that will significantly decrease draw-calls.
// currently, keep it simple.
// #define ARC_BRUSH_CPU_TRANSFORM

namespace arc {

struct brush {
    color vertex_color[4]{};
    std::stack<transform> tstack_;
    camera camera_;
    graph_state state_;
    std::shared_ptr<program> default_colored_;
    std::shared_ptr<program> default_textured_;
    complex_buffer* wbuf = nullptr;
    mesh* mesh_root_ = nullptr;
    bool is_in_mesh_ = false;
    // when true, the brush will clear the buffer when flushed.
    bool clear_when_flush_ = true;

    brush();

    graph_state& current_state();
    void cl_norm();
    void cl_set(const color& col);
    void cl_mrg(const color& col);
    void cl_mrg(double v);
    void ts_push();
    void ts_pop();
    void ts_load(const transform& t);
    void ts_trs(const vec2& v);
    void ts_scl(const vec2& v);
    void ts_shr(const vec2& v);
    void ts_rot(double r);
    void ts_rot(const vec2& v, double r);
    transform get_combined_transform();

    void flush();
    void assert_mode(graph_mode mode);
    void assert_texture(std::shared_ptr<texture> tex);
    void use_camera(const camera& cam);
    void use_program(std::shared_ptr<program> program);
    void use_state(const graph_state& sts);

    void draw_texture(std::shared_ptr<texture> tex, const quad& dst, const quad& src, uint8_t flag = brush_flag::NO);
    void draw_texture(std::shared_ptr<texture> tex, const quad& dst, uint8_t flag = brush_flag::NO);
    void draw_rect(const quad& dst);
    void draw_rect_outline(const quad& dst);
    void draw_triagle(const vec2& p1, const vec2& p2, const vec2& p3);
    void draw_line(const vec2& p1, const vec2& p2);
    void draw_point(const vec2& p);
    void draw_oval(const quad& dst, int segs = 16);
    void draw_oval_outline(const quad& dst, int segs = 16);

    void clear(const color& col);
    void viewport(const quad& quad);
    void scissor(const quad& quad);
    void scissor_end();
    void use_blend(blend_mode mode);
};

}  // namespace arc