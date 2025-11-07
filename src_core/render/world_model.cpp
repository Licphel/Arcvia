#include "render/world_model.h"

#include "gfx/device.h"
#include "gfx/fbuf.h"
#include "gfx/shader.h"
#include "render/chunk_model.h"
#include "render/liquid_model.h"
#include "world/chunk.h"
#include "world/dimh.h"
#include "world/liquid.h"


namespace arc {

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

void wrd::retry() {
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

void wrd::render(brush* brush, dimension* dim, const quad& cam) {
    int cy0 = std::round(cam.y - 1);
    int cy1 = std::round(cam.prom_y() + 1);
    int cx0 = std::round(cam.x - 1);
    int cx1 = std::round(cam.prom_x() + 1);

    // chunk layer render macro
#define ARC_RCLVL_(layer)                                                                                       \
    for (auto& kv : dim->chunk_map) {                                                                           \
        auto& chunk_ = kv.second;                                                                               \
        if (chunk_->min_x > cx1 || chunk_->min_y > cy1 || chunk_->max_x < cx0 || chunk_->max_y < cy0) continue; \
        if (chunk_->model->built[static_cast<int>(layer)]) chunk_->model->render(brush, layer);                 \
    }
    // end

    fb_world_back_->retry(brush);
    ARC_RCLVL_(chunk_mesh_layer::back_block);
    ARC_RCLVL_(chunk_mesh_layer::back_block_border);
    fb_world_back_->record(brush);
    fb_world_front_->retry(brush);
    ARC_RCLVL_(chunk_mesh_layer::furniture);

    // entity rendering
    quad box_find = cam;
    box_find.inflate(ARC_RENDER_ENTITY_FIND, ARC_RENDER_ENTITY_FIND);
    const auto& found = dim_util::get_intersected_entities(dim, box_find);

    for (auto& e : found) {
        // todo
        brush->draw_rect_outline(e->lerped_box_());
    }

    // liquid rendering
    for (auto& kv : dim->chunk_map) {
        auto& chunk_ = kv.second;
        if (chunk_->min_x > cx1 || chunk_->min_y > cy1 || chunk_->max_x < cx0 || chunk_->max_y < cy0) continue;
        scan_cxc(chunk_.get(), [&](const pos2i& pos) {
            liquid_stack qstack = chunk_->find_liquid_stack(pos);
            if (!qstack.is_empty())
                qstack.liquid->model->make_liquid(brush, qstack.liquid->model, dim, qstack, static_cast<pos2d>(pos));
        });
    }

    ARC_RCLVL_(chunk_mesh_layer::block);
    ARC_RCLVL_(chunk_mesh_layer::block_border);
    ARC_RCLVL_(chunk_mesh_layer::overlay);
    fb_world_back_->record(brush);
}

void wrd::submit(brush* brush, dimension* dim) {
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