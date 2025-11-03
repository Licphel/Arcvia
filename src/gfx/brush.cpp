#include "gfx/brush.h"

#include "core/def.h"
#include "core/log.h"
#include "core/math.h"
#include "gfx/buffer.h"
#include "gfx/device.h"
#include "gfx/image.h"
#include "gfx/mesh.h"
#include "gfx/shader.h"

// clang-format off
#include <gl/glew.h>
#include <gl/gl.h>
// clang-format on

namespace arc {

static uint16_t to_half_(float f) {
    union {
        float f;
        uint32_t u;
    } v = {f};
    uint32_t s = (v.u >> 31) & 0x1;
    uint32_t e = (v.u >> 23) & 0xFF;
    uint32_t m = v.u & 0x7FFFFF;
    if (e == 0xFF) return uint16_t((s << 15) | 0x7C00 | (m ? 1 : 0));
    if (!e) return uint16_t((s << 15) | (m >> 13));
    int32_t E = int32_t(e) - 127 + 15;
    if (E > 31) E = 31;
    if (E < 0) E = 0;
    return uint16_t((s << 15) | (E << 10) | (m >> 13));
}

void w_half_(complex_buffer* buf, const color& col) {
    buf->vtx(to_half_(col.r)).vtx(to_half_(col.g)).vtx(to_half_(col.b)).vtx(to_half_(col.a));
}

brush::brush() {
    cl_norm();
    ts_push();
    default_colored_ = program::make(builtin_program_type::colored);
    default_textured_ = program::make(builtin_program_type::textured);
}

graph_state& brush::current_state() { return state_; }

void brush::cl_norm() { cl_set({}); }

void brush::cl_set(const color& col) { vertex_color[0] = vertex_color[1] = vertex_color[2] = vertex_color[3] = col; }

void brush::cl_mrg(const color& col) {
    vertex_color[0] = vertex_color[0] * col;
    vertex_color[1] = vertex_color[1] * col;
    vertex_color[2] = vertex_color[2] * col;
    vertex_color[3] = vertex_color[3] * col;
}

void brush::cl_mrg(double v) {
    vertex_color[0] = vertex_color[0] * v;
    vertex_color[1] = vertex_color[1] * v;
    vertex_color[2] = vertex_color[2] * v;
    vertex_color[3] = vertex_color[3] * v;
}

void brush::ts_push() {
#ifndef ARC_BRUSH_CPU_TRANSFORM
    flush();
#endif
    tstack_.push(transform());
}

void brush::ts_pop() {
#ifndef ARC_BRUSH_CPU_TRANSFORM
    flush();
#endif
    tstack_.pop();
}

void brush::ts_load(const transform& t) {
#ifndef ARC_BRUSH_CPU_TRANSFORM
    flush();
#endif
    tstack_.push(t);
}

void brush::ts_trs(const vec2& v) {
#ifndef ARC_BRUSH_CPU_TRANSFORM
    flush();
#endif
    tstack_.top().translate(v.x, v.y);
}

void brush::ts_scl(const vec2& v) {
#ifndef ARC_BRUSH_CPU_TRANSFORM
    flush();
#endif
    tstack_.top().scale(v.x, v.y);
}

void brush::ts_shr(const vec2& v) {
#ifndef ARC_BRUSH_CPU_TRANSFORM
    flush();
#endif
    tstack_.top().shear(v.x, v.y);
}

void brush::ts_rot(double r) {
#ifndef ARC_BRUSH_CPU_TRANSFORM
    flush();
#endif
    tstack_.top().rotate(r);
}

void brush::ts_rot(const vec2& v, double r) {
#ifndef ARC_BRUSH_CPU_TRANSFORM
    flush();
#endif
    transform& trs = tstack_.top();
    trs.translate(v.x, v.y);
    trs.rotate(r);
    trs.translate(-v.x, -v.y);
}

void brush::use_camera(const camera& cam) {
    flush();
    camera_ = cam;
    viewport(cam.viewport);
}

void brush::use_program(std::shared_ptr<program> program) {
    if (state_.prog != program) {
        flush();
        state_.prog = program;
    }
}

void brush::use_state(const graph_state& sts) {
    flush();
    use_program(state_.prog);
    assert_texture(state_.texture);
    assert_mode(state_.mode);
    state_ = sts;
};

transform brush::get_combined_transform() {
    transform cpy = camera_.combined_out_t;
    return cpy.mul(tstack_.top());
}

void brush::flush() {
    if (wbuf == nullptr) return;

    auto buf = wbuf;
    auto msh = mesh_root_;

    if (buf->vertex_buf.size() <= 0) return;

    // ignore empty-flushing.
    if (is_in_mesh_)
        print_throw(log_level::fatal,
                    "it seems that somewhere the brush is flushed in a mesh. the "
                    "state cannot be consistent!");

    std::shared_ptr<program> program_used;
    if (state_.prog != nullptr && state_.prog->program_id_ != 0)
        program_used = state_.prog;
    else
        switch (state_.mode) {
            case graph_mode::textured_quad:
                program_used = default_textured_;
                break;
            default:
                program_used = default_colored_;
                break;
        }

    glBindVertexArray(msh->vao_);
    glBindBuffer(GL_ARRAY_BUFFER, msh->vbo_);
    if (buf->dirty) {
        if (buf->vcap_changed_)
            glBufferData(GL_ARRAY_BUFFER, buf->vertex_buf.capacity(), buf->vertex_buf.data(), GL_DYNAMIC_DRAW);
        else
            glBufferSubData(GL_ARRAY_BUFFER, 0, buf->vertex_buf.size(), buf->vertex_buf.data());
    }
    buf->vcap_changed_ = false;

    glUseProgram(program_used->program_id_);
    if (program_used->callback_setup != nullptr) program_used->callback_setup(program_used.get());

    if (state_.callback_uniform != nullptr) state_.callback_uniform(program_used.get());

    if (program_used->cached_uniforms.size() == 0)
        print_throw(log_level::fatal, "please cache at lease a uniform u_proj.");
    else
#ifdef ARC_BRUSH_CPU_TRANSFORM
        program_used->cached_uniforms[0].set(camera_.combined_out_t);
#else
        program_used->cached_uniforms[0].set(get_combined_transform());
#endif

    if (state_.mode == graph_mode::textured_quad || state_.mode == graph_mode::colored_quad) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, msh->ebo_);
        if (buf->icap_changed_)
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, buf->index_buf.capacity() * 4, buf->index_buf.data(), GL_STATIC_DRAW);
        else
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, buf->index_buf.size() * 4, buf->index_buf.data());
    }
    buf->icap_changed_ = false;
    buf->dirty = false;

    switch (state_.mode) {
        case graph_mode::textured_quad:
            state_.texture->bind_(1);
            glDrawElements(GL_TRIANGLES, buf->index_count, GL_UNSIGNED_INT, 0);
            break;
        case graph_mode::colored_quad:
            glDrawElements(GL_TRIANGLES, buf->index_count, GL_UNSIGNED_INT, 0);
            break;
        case graph_mode::colored_line:
            glDrawArrays(GL_LINES, 0, buf->vertex_count);
            break;
        case graph_mode::colored_point:
            glDrawArrays(GL_POINTS, 0, buf->vertex_count);
            break;
        case graph_mode::colored_triangle:
            glDrawArrays(GL_TRIANGLES, 0, buf->vertex_count);
            break;
        default:
            print_throw(log_level::fatal, "uknown graphics mode.");
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glUseProgram(0);

    if (clear_when_flush_) buf->clear();
}

void brush::assert_mode(graph_mode mode) {
    if (state_.mode != mode) {
        flush();
        state_.mode = mode;
    }
}

static unsigned int get_tex_root_(std::shared_ptr<texture> tex) {
    if (tex == nullptr) return 0;
    return tex->texture_id_;
}

void brush::assert_texture(std::shared_ptr<texture> tex) {
    if (get_tex_root_(state_.texture) != get_tex_root_(tex)) {
        flush();
        state_.texture = tex;
    }
}

void brush::draw_texture(std::shared_ptr<texture> tex, const quad& dst, const quad& src, uint8_t flag) {
    if (tex == nullptr) return;
    auto buf = wbuf;

    assert_mode(graph_mode::textured_quad);
    assert_texture(tex);

    float u = (src.x + tex->u) / tex->full_width;
    float v = (src.y + tex->v) / tex->full_height;
    float u2 = (src.prom_x() + tex->u) / tex->full_width;
    float v2 = (src.prom_y() + tex->v) / tex->full_height;

    if (flag & brush_flag::FLIP_X) std::swap(u, u2);
#ifdef ARC_Y_IS_DOWN
    if (!(flag & brush_flag::FLIP_Y) ^ tex->is_framebuffer_)
#else
    if ((flag & brush_flag::FLIP_Y) ^ tex->is_framebuffer_)
#endif
        std::swap(v, v2);

    float x = dst.x, y = dst.y, w = dst.width, h = dst.height;
    float x1 = x, y1 = y, x2 = x + w, y2 = y + h;
#ifdef ARC_BRUSH_CPU_TRANSFORM
    auto& ts = tstack_.top();
    ts.apply(x1, y1);
    ts.apply(x2, y2);
#endif

    buf->vtx(x2).vtx(y2);
    w_half_(buf, vertex_color[2]);
    buf->vtx(u2).vtx(v);
    buf->vtx(x2).vtx(y1);
    w_half_(buf, vertex_color[3]);
    buf->vtx(u2).vtx(v2);
    buf->vtx(x1).vtx(y1);
    w_half_(buf, vertex_color[0]);
    buf->vtx(u).vtx(v2);
    buf->vtx(x1).vtx(y2);
    w_half_(buf, vertex_color[1]);
    buf->vtx(u).vtx(v);

    buf->end_quad();
}

void brush::draw_texture(std::shared_ptr<texture> tex, const quad& dst, uint8_t flag) {
    if (tex != nullptr) draw_texture(tex, dst, quad(0.0, 0.0, tex->width, tex->height), flag);
}

void brush::draw_rect(const quad& dst) {
    auto buf = wbuf;

    assert_mode(graph_mode::colored_quad);

    float x = dst.x, y = dst.y, w = dst.width, h = dst.height;
    float x1 = x, y1 = y, x2 = x + w, y2 = y + h;
#ifdef ARC_BRUSH_CPU_TRANSFORM
    auto& ts = tstack_.top();
    ts.apply(x1, y1);
    ts.apply(x2, y2);
#endif

    buf->vtx(x2).vtx(y2);
    w_half_(buf, vertex_color[2]);
    buf->vtx(x2).vtx(y1);
    w_half_(buf, vertex_color[3]);
    buf->vtx(x1).vtx(y1);
    w_half_(buf, vertex_color[0]);
    buf->vtx(x1).vtx(y2);
    w_half_(buf, vertex_color[1]);

    buf->end_quad();
}

void brush::draw_rect_outline(const quad& dst) {
    float x = dst.x;
    float y = dst.y;
    float width = dst.width;
    float height = dst.height;

    draw_line({x, y}, {x + width, y});
    draw_line({x, y}, {x, y + height});
    draw_line({x + width, y}, {x + width, y + height});
    draw_line({x, y + height}, {x + width, y + height});
}

void brush::draw_triagle(const vec2& p1, const vec2& p2, const vec2& p3) {
    auto buf = wbuf;

    assert_mode(graph_mode::colored_triangle);

    float x1 = p1.x, y1 = p1.y, x2 = p2.x, y2 = p2.y, x3 = p3.x, y3 = p3.y;
#ifdef ARC_BRUSH_CPU_TRANSFORM
    auto& ts = tstack_.top();
    ts.apply(x1, y1);
    ts.apply(x2, y2);
    ts.apply(x3, y3);
#endif

    buf->vtx(x1).vtx(y1);
    w_half_(buf, vertex_color[0]);
    buf->vtx(x2).vtx(y2);
    w_half_(buf, vertex_color[1]);
    buf->vtx(x3).vtx(y3);
    w_half_(buf, vertex_color[2]);
    buf->new_vertex(3);
}

void brush::draw_line(const vec2& p1, const vec2& p2) {
    auto buf = wbuf;

    assert_mode(graph_mode::colored_line);

    float x1 = p1.x, y1 = p1.y, x2 = p2.x, y2 = p2.y;
#ifdef ARC_BRUSH_CPU_TRANSFORM
    auto& ts = tstack_.top();
    ts.apply(x1, y1);
    ts.apply(x2, y2);
#endif

    buf->vtx(x1).vtx(y1);
    w_half_(buf, vertex_color[0]);
    buf->vtx(x2).vtx(y2);
    w_half_(buf, vertex_color[1]);
    buf->new_vertex(2);
}

void brush::draw_point(const vec2& p) {
    auto buf = wbuf;

    assert_mode(graph_mode::colored_point);

    float x1 = p.x, y1 = p.y;
#ifdef ARC_BRUSH_CPU_TRANSFORM
    auto& ts = tstack_.top();
    ts.apply(x1, y1);
#endif

    buf->vtx(x1).vtx(y1);
    w_half_(buf, vertex_color[0]);
    buf->new_vertex(1);
}

void brush::draw_oval(const quad& dst, int segs) {
    if (segs <= 1) print_throw(log_level::fatal, "at least drawing an oval needs 2 segments.");

    float x = dst.center_x();
    float y = dst.center_y();
    float width = dst.width;
    float height = dst.height;

    float onerad = 2 * 3.1415926535 / segs;
    for (float i = 0; i < 2 * 3.1415926535; i += onerad) {
        float x1 = x + cosf(i) * width;
        float y1 = y + sinf(i) * height;
        float x2 = x + cosf(i + onerad) * width;
        float y2 = y + sinf(i + onerad) * height;
        draw_triagle({x, y}, {x1, y1}, {x2, y2});
    }
}

void brush::draw_oval_outline(const quad& dst, int segs) {
    if (segs <= 1) print_throw(log_level::fatal, "at least drawing an oval needs 2 segments.");

    float x = dst.center_x();
    float y = dst.center_y();
    float width = dst.width;
    float height = dst.height;

    float onerad = 2 * 3.1415926535 / segs;
    for (float i = 0; i < 2 * 3.1415926535; i += onerad) {
        float x1 = x + cosf(i) * width;
        float y1 = y + sinf(i) * height;
        float x2 = x + cosf(i + onerad) * width;
        float y2 = y + sinf(i + onerad) * height;
        draw_line({x1, y1}, {x2, y2});
    }
}

void brush::clear(const color& col) {
    glClearColor(col.r, col.g, col.b, col.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void brush::viewport(const quad& quad) {
    flush();
    double y = quad.y;
#ifdef ARC_Y_IS_DOWN
    double h = tk_get_size().y;
    y = h - quad.height - 2 * y;
#endif
    glViewport(quad.x, y, quad.width, quad.height);
}

void brush::scissor(const quad& quad) {
    flush();

    double x = camera_.project_x(quad.x);
    double y = camera_.project_y(quad.y);
    double xp = camera_.project_x(quad.prom_x());
    double yp = camera_.project_y(quad.prom_y());

    // transform the point at left-top to left-bottom.
#ifdef ARC_Y_IS_DOWN
    double h = tk_get_size().y;
    double hrct = yp - y;
    y = h - hrct - y;
    yp = y + hrct;
#endif

    glScissor(x - 0.5, y - 0.5, xp - x + 1.0, yp - y + 1.0);
    glEnable(GL_SCISSOR_TEST);
}

void brush::scissor_end() {
    flush();
    vec2 v = tk_get_size();
    glScissor(0, 0, v.x + 1.0, v.y + 1.0);
    glDisable(GL_SCISSOR_TEST);
}

void brush::use_blend(blend_mode mode) {
    flush();
    if (mode == blend_mode::normal)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    else if (mode == blend_mode::add)
        glBlendFunc(GL_SRC_ALPHA, GL_SRC_ALPHA);
}

}  // namespace arc