#include "render/light.h"

#include <memory>
#include <utility>

#include "core/thrp.h"
#include "gfx/device.h"
#include "gfx/fbuf.h"
#include "gfx/shader.h"
#include "light.h"
#include "render/chunk_model.h"
#include "world/block.h"
#include "world/dim.h"
#include "world/dimh.h"
#include "world/liquid.h"
#include "world/pos.h"


namespace arc {

static std::unique_ptr<float[]> empty_lm = std::make_unique<float[]>(7);
ldata_ ldata_::empty = ldata_(nullptr, empty_lm.get());

void ldata_::ao_block(int x, int y) {
    block_behavior* b = fast_get_block(x, y);

    const float ao_sim = 0.1;

    if (b->shape == block_shape::opaque) {
        float v = 1 - ao_sim * 1.5;
        data[3] = v;
        data[4] = v;
        data[5] = v;
        data[6] = v;
    } else {
        int c = 0;
        block_behavior* b0 = fast_get_block(x - 1, y - 1);
        block_behavior* b1 = fast_get_block(x - 1, y);
        block_behavior* b2 = fast_get_block(x - 1, y + 1);
        block_behavior* b3 = fast_get_block(x, y - 1);
        block_behavior* b4 = fast_get_block(x, y + 1);
        block_behavior* b5 = fast_get_block(x + 1, y - 1);
        block_behavior* b6 = fast_get_block(x + 1, y);
        block_behavior* b7 = fast_get_block(x + 1, y + 1);
        bool bcc1 = b1->shape == block_shape::opaque;
        bool bcc3 = b3->shape == block_shape::opaque;
        bool bcc4 = b4->shape == block_shape::opaque;
        bool bcc6 = b6->shape == block_shape::opaque;

        if (b0->shape == block_shape::opaque) c++;
        if (bcc1) c++;
        if (bcc3) c++;
        data[3] = 1 - c * ao_sim;

        c = 0;
        if (bcc1) c++;
        if (b2->shape == block_shape::opaque) c++;
        if (bcc4) c++;
        data[4] = 1 - c * ao_sim;

        c = 0;
        if (bcc4) c++;
        if (bcc6) c++;
        if (b7->shape == block_shape::opaque) c++;
        data[5] = 1 - c * ao_sim;

        c = 0;
        if (bcc3) c++;
        if (b5->shape == block_shape::opaque) c++;
        if (bcc6) c++;
        data[6] = 1 - c * ao_sim;
    }
}

void pipe_ft(ldata_& self, color* color, int x, int y, int pipe, bool back) {
    const float back_darkened = 0.75;

    auto engine = self.engine;
    float arr1 = engine->at(x - 1, y).data[pipe];
    float arr2 = engine->at(x + 1, y).data[pipe];
    float arr3 = engine->at(x, y - 1).data[pipe];
    float arr4 = engine->at(x, y + 1).data[pipe];
    float arr5 = engine->at(x - 1, y - 1).data[pipe];
    float arr6 = engine->at(x + 1, y + 1).data[pipe];
    float arr7 = engine->at(x - 1, y + 1).data[pipe];
    float arr8 = engine->at(x + 1, y - 1).data[pipe];
    float l0 = self.data[pipe];

    float v0 = (l0 + arr1 + arr3 + arr5) / 4;
    float v1 = (l0 + arr1 + arr4 + arr7) / 4;
    float v2 = (l0 + arr2 + arr4 + arr6) / 4;
    float v3 = (l0 + arr2 + arr3 + arr8) / 4;

    switch (pipe) {
        case 0:
            color[0].r = v0 * (back ? back_darkened * self.data[3] : 1);
            color[1].r = v1 * (back ? back_darkened * self.data[4] : 1);
            color[2].r = v2 * (back ? back_darkened * self.data[5] : 1);
            color[3].r = v3 * (back ? back_darkened * self.data[6] : 1);
            break;
        case 1:
            color[0].g = v0 * (back ? back_darkened * self.data[3] : 1);
            color[1].g = v1 * (back ? back_darkened * self.data[4] : 1);
            color[2].g = v2 * (back ? back_darkened * self.data[5] : 1);
            color[3].g = v3 * (back ? back_darkened * self.data[6] : 1);
            break;
        case 2:
            color[0].b = v0 * (back ? back_darkened * self.data[3] : 1);
            color[1].b = v1 * (back ? back_darkened * self.data[4] : 1);
            color[2].b = v2 * (back ? back_darkened * self.data[5] : 1);
            color[3].b = v3 * (back ? back_darkened * self.data[6] : 1);
            color[0].a = 1;
            color[1].a = 1;
            color[2].a = 1;
            color[3].a = 1;
            break;
    }
}

void ldata_::lit_block(color* color, int x, int y, bool back) {
    pipe_ft(*this, color, x, y, 0, back);
    pipe_ft(*this, color, x, y, 1, back);
    pipe_ft(*this, color, x, y, 2, back);
}

void light_engine::init(dimension* dim) {
    this->dim = dim;
    front_map = mesh::make();
    back_map = mesh::make();
    front_map_used = mesh::make();
    back_map_used = mesh::make();
    lm = new float[sx * sy * 7]();
    lm_stable = new float[sx * sy * 7]();
}

color light_engine::color_stably(float x, float y) {
    int ix = std::floor(x);
    int iy = std::floor(y);

    ldata_ ldat = at_stably(ix, iy);
    color rgb;
    rgb.r = ldat.data[0];
    rgb.g = ldat.data[1];
    rgb.b = ldat.data[2];
    return rgb;
}

void spread_pipe(ldata_& self, chunk* chunk, int x, int y, int idx) {
    auto engine = self.engine;
    float l2 = engine->at(x - 1, y).data[idx];
    float l3 = engine->at(x + 1, y).data[idx];
    float l4 = engine->at(x, y - 1).data[idx];
    float l5 = engine->at(x, y + 1).data[idx];
    float l1 = std::max(l2, std::max(l3, std::max(l4, l5)));

    if (l1 <= light_engine::dark_luminance) {
        self.data[idx] = 0;  // No need to calc in this case.
    } else {
        auto pos = pos2i(x, y);
        block_behavior* block = chunk->find_block(pos);
        liquid_stack qstack = chunk->find_liquid_stack(pos);
        l1 = block->block_light ? block->block_light(chunk->dim, pos, idx, l1) : block->shape.block_light(l1);
        l1 = qstack.liquid->block_light ? qstack.liquid->block_light(chunk->dim, pos, qstack, idx, l1)
                                        : block_shape::transparent.block_light(l1);
        self.data[idx] = l1;
    }
}

void light_engine::spread(int x, int y) {
    chunk* chunk = fast_get_chunk(x, y);
    if (chunk == nullptr) return;

    ldata_ data = at(x, y);

    spread_pipe(data, chunk, x, y, 0);
    spread_pipe(data, chunk, x, y, 1);
    spread_pipe(data, chunk, x, y, 2);
}

float light_engine::get_block_shed(chunk* chunk, int x, int y, int pipe) {
    auto pos = pos2i(x, y);
    block_behavior* b1 = chunk->find_block(pos);
    block_behavior* b2 = chunk->find_back_block(pos);
    liquid_stack qstack = chunk->find_liquid_stack(pos);

    float l1 = b1->cast_light ? b1->cast_light(dim, pos, pipe) : 0.0;
    float l2 = b2->cast_light ? b2->cast_light(dim, pos, pipe) : 0.0;
    float l3 = qstack.liquid->cast_light ? qstack.liquid->cast_light(dim, pos, qstack, pipe) : 0.0;

    return std::max(l1, std::max(l2, l3)) * amp;
}

float light_engine::get_sky_shed(chunk* chunk, int x, int y, int pipe) {
    if (y > dimension::sea_level + 5) return 0;

    auto pos = pos2i(x, y);
    block_behavior* b1 = chunk->find_block(pos);
    block_behavior* b2 = chunk->find_back_block(pos);
    liquid_stack qstack = chunk->find_liquid_stack(pos);
    float v0 = y < dimension::sea_level ? 1.0 : std::max(0.0, 1 - (y - dimension::sea_level) * 0.25);

    if (b1->shape == block_shape::vaccum && b2->shape == block_shape::vaccum) {
        float fv = qstack.liquid->block_light ? qstack.liquid->block_light(dim, pos, qstack, pipe, v0)
                                              : block_shape::transparent.block_light(v0);
        return fv * sunlight[pipe] * amp;
    }

    if (b1->shape != block_shape::opaque && b2->shape != block_shape::opaque) {
        float fv = b2->block_light ? b2->block_light(dim, pos, pipe, v0) : b2->shape.block_light(v0);
        return fv * sunlight[pipe] * amp;
    }

    return 0.0;
}

void light_engine::tick(const quad& cam) {
    lorix = std::floor(cam.center_x()) - sx / 2.0;
    loriy = std::floor(cam.center_y()) - sy / 2.0;

    color sun = {1, 1, 1, 1};
    sunlight[0] = sun.r;
    sunlight[1] = sun.g;
    sunlight[2] = sun.b;

    if (end_lit) {
        // swap surface buffer and back buffer (back buffer is used to render).
        std::swap(front_map_used, front_map);
        std::swap(back_map_used, back_map);
        std::swap(lm, lm_stable);
        end_lit = false;
        start_lit = false;
    }

    if (!start_lit) {
        start_lit = true;
        thread_pool::execute([this, cam]() {
            calculate(cam);
            render_meshes(cam);
            end_lit = true;
        });
    }
}

void light_engine::lit_smooth(float x, float y, float v1, float v2, float v3) {
    if (v1 <= dark_luminance && v2 <= dark_luminance && v3 <= dark_luminance) return;

    const int r = 3;
    int lx = std::floor(x);
    int ly = std::floor(y);

    for (int tx = lx - r; tx < lx + r; tx += 1) {
        for (int ty = ly - r; ty < ly + r; ty += 1) {
            float dist2 = std::pow(tx - x, 2) + std::pow(ty - y, 2);
            if (dist2 < 1) dist2 = 1;
            lit(tx, ty, v1 / dist2, v2 / dist2, v3 / dist2);
        }
    }
}

void light_engine::lit(int x, int y, float v1, float v2, float v3) {
    chunk* chunk = fast_get_chunk(x, y);
    if (chunk == nullptr) return;

    ldata_ data = at(x, y);
    data.data[0] = std::max(data.data[0], v1 * amp);
    data.data[1] = std::max(data.data[1], v2 * amp);
    data.data[2] = std::max(data.data[2], v3 * amp);
}

void light_engine::calculate(const quad& cam) {
    float spd = ult_max / unit / 2.0;

    int x0 = static_cast<int>(cam.x - spd);
    int y0 = static_cast<int>(cam.y - spd);
    int x1 = static_cast<int>(cam.prom_x() + spd);
    int y1 = static_cast<int>(cam.prom_y() + spd);

    for (int x = x0 - 1; x <= x1 + 1; x++) {
        for (int y = y0 - 1; y <= y1 + 1; y++) {
            chunk* chunk = fast_get_chunk(x, y);
            if (chunk == nullptr) continue;

            ldata_ data = at(x, y);
            data.data[0] = std::max(get_block_shed(chunk, x, y, 0), get_sky_shed(chunk, x, y, 0));
            data.data[1] = std::max(get_block_shed(chunk, x, y, 1), get_sky_shed(chunk, x, y, 1));
            data.data[2] = std::max(get_block_shed(chunk, x, y, 2), get_sky_shed(chunk, x, y, 2));

            if (chunk->model->ao_rebuild) data.ao_block(x, y);
        }
    }

    quad aabb = quad();
    aabb.resize(x1 - x0, y1 - y0);
    aabb.locate_center((x1 + x0) / 2.0, (y1 + y0) / 2.0);

    for (entity& e : dimh::get_intersected_entities(dim, aabb)) {
        if (!e.cast_light) continue;
        float r = e.cast_light(&e, 0);
        float g = e.cast_light(&e, 1);
        float b = e.cast_light(&e, 2);

        lit_smooth(e.motion->pos.x, e.motion->pos.y, r, g, b);
    }

    for (int x = x1; x >= x0; x--)
        for (int y = y1; y >= y0; y--) spread(x, y);
    for (int x = x0; x <= x1; x++)
        for (int y = y0; y <= y1; y++) spread(x, y);
    for (int x = x0; x <= x1; x++)
        for (int y = y1; y >= y0; y--) spread(x, y);
    for (int x = x1; x >= x0; x--)
        for (int y = y0; y <= y1; y++) spread(x, y);
}

ldata_ light_engine::at(int x, int y) {
    x -= lorix;
    y -= loriy;
    if (x < 0 || x >= sx || y < 0 || y >= sy) return ldata_::empty;
    return ldata_(this, lm + (x + y * sx) * 7);
}

ldata_ light_engine::at_stably(int x, int y) {
    x -= lorix;
    y -= loriy;
    if (x < 0 || x >= sx || y < 0 || y >= sy) return ldata_::empty;
    return ldata_(this, lm_stable + (x + y * sx) * 7);
}

static void populate_lm_mesh(light_engine* engine, const quad& cam, std::shared_ptr<mesh> mesh, float* lm, bool back) {
    const int overdraw = 8;
    int cy0 = std::round(cam.y - overdraw);
    int cy1 = std::round(cam.prom_y() + overdraw);
    int cx0 = std::round(cam.x - overdraw);
    int cx1 = std::round(cam.prom_x() + overdraw);

    brush* brush_ = mesh->retry();

    for (int x = cx0; x <= cx1; x++) {
        for (int y = cy0; y <= cy1; y++) {
            ldata_ ldat = engine->at(x, y);
            ldat.lit_block(brush_->vertex_color, x, y, back);
            brush_->draw_rect(quad(x, y, 1, 1));
        }
    }

    mesh->record();
}

void light_engine::render_meshes(const quad& cam) {
    populate_lm_mesh(this, cam, back_map, lm, true);
    populate_lm_mesh(this, cam, front_map, lm, false);
}

static const std::string dvert_lmg_textured_ =
    "#version 330 core\n"
    "layout(location = 0) in vec2 i_position;\n"
    "layout(location = 1) in vec4 i_color;\n"
    "layout(location = 2) in vec2 i_texCoord;\n"
    "out vec4 o_color;\n"
    "out vec2 o_texCoord;\n"
    "uniform mat4 u_proj;\n"
    "void main() {\n"
    "    o_color = i_color;\n"
    "    o_texCoord = i_texCoord;\n"
    "    gl_Position = u_proj * vec4(i_position.x, i_position.y, 0.0, 1.0);\n"
    "}";

static const std::string dfrag_lmg_textured_ =
    "#version 330 core\n"
    "in vec4 o_color;\n"
    "in vec2 o_texCoord;\n"
    "out vec4 fragColor;\n"
    "uniform sampler2D u_tex;\n"
    "uniform sampler2D u_lighttex;\n"
    "void main() {\n"
    "    fragColor = o_color * texture(u_tex, o_texCoord) * texture(u_lighttex, o_texCoord);\n"
    "}";

static bool init_ = false;
static std::shared_ptr<program> lmg_shader_prog_;
static std::shared_ptr<framebuffer> fb_back_;
static std::shared_ptr<framebuffer> fb_front_;
static std::shared_ptr<framebuffer> fb_world_back_;
static std::shared_ptr<framebuffer> fb_world_front_;

void lwrd::world_render_begin() {
    if (!init_) {
        init_ = true;
        lmg_shader_prog_ = program::make(dvert_lmg_textured_, dfrag_lmg_textured_, [](program* program) {
            program->get_attrib(0).layout(shader_vertex_data_type::f32, 2, 24, 0);
            program->get_attrib(1).layout(shader_vertex_data_type::f16, 4, 24, 8);
            program->get_attrib(2).layout(shader_vertex_data_type::f32, 2, 24, 16);

            if (program->cached_uniforms.size() > 0) return;
            program->cache_uniform("u_proj");                          // 0
            program->cache_uniform("u_tex").set_texture_unit(1);       // 1
            program->cache_uniform("u_lighttex").set_texture_unit(2);  // 2
        });
        fb_back_ = framebuffer::make();
        fb_front_ = framebuffer::make();
        fb_world_back_ = framebuffer::make();
        fb_world_front_ = framebuffer::make();
    }
}

std::shared_ptr<framebuffer> lwrd::world_back_buffer() { return fb_world_back_; }
std::shared_ptr<framebuffer> lwrd::world_front_buffer() { return fb_world_front_; }

void lwrd::world_render(brush* brush, dimension* dim) {
    fb_back_->retry(brush);
    dim->light_executor->back_map_used->draw(brush);
    fb_back_->record(brush);

    fb_front_->retry(brush);
    dim->light_executor->front_map_used->draw(brush);
    fb_front_->record(brush);

    vec2 s = tk_get_size();
    quad ddm = quad(0, 0, s.x, s.y);

    brush->use_camera(camera::normal());
    brush->use_program(lmg_shader_prog_);
    brush->current_state().callback_uniform = [&](program* program) { fb_back_->output->bind_(2); };
    brush->draw_texture(fb_world_back_->output, ddm);
    brush->flush();
    brush->current_state().callback_uniform = [&](program* program) { fb_front_->output->bind_(2); };
    brush->draw_texture(fb_world_front_->output, ddm);
    brush->use_program(nullptr);
    brush->current_state().callback_uniform = nullptr;
}

}  // namespace arc