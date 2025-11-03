#include "gfx/brush.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/image.h"
#include "gfx/mesh.h"
#include "lua/lua.h"

namespace arc {

void lua_bind_gfx(lua_state& lua) {
    auto _n = lua_make_table();

    // brush
    auto brush_type = lua_new_usertype<brush>(_n, "brush", lua_native);
    brush_type["draw_texture"] =
        lua_combine([](brush* self, std::shared_ptr<texture> tex, const quad& dst, const quad& src,
                       uint8_t flag) { self->draw_texture(tex, dst, src, flag); },
                    [](brush* self, std::shared_ptr<texture> tex, const quad& dst, const quad& src) {
                        self->draw_texture(tex, dst, src);
                    },
                    [](brush* self, std::shared_ptr<texture> tex, const quad& dst, uint8_t flag) {
                        self->draw_texture(tex, dst, flag);
                    },
                    [](brush* self, std::shared_ptr<texture> tex, const quad& dst) { self->draw_texture(tex, dst); });
    brush_type["draw_rect"] = &brush::draw_rect;
    brush_type["draw_oval"] = &brush::draw_oval;
    brush_type["draw_rect_outline"] = &brush::draw_rect_outline;
    brush_type["draw_oval_outline"] = &brush::draw_oval_outline;
    brush_type["draw_triangle"] = &brush::draw_triagle;
    brush_type["draw_line"] = &brush::draw_line;
    brush_type["draw_point"] = &brush::draw_point;
    brush_type["cl_norm"] = &brush::cl_norm;
    brush_type["cl_set"] = &brush::cl_set;
    brush_type["cl_mrg"] =
        lua_combine(TYPE_FN_(brush, void, cl_mrg, const color&), TYPE_FN_(brush, void, cl_mrg, double));
    brush_type["vertex_color"] = &brush::vertex_color;
    brush_type["ts_push"] = &brush::ts_push;
    brush_type["ts_pop"] = &brush::ts_pop;
    brush_type["ts_trs"] = &brush::ts_trs;
    brush_type["ts_rot"] =
        lua_combine(TYPE_FN_(brush, void, ts_rot, const vec2&, double), TYPE_FN_(brush, void, ts_rot, double));
    brush_type["ts_scl"] = &brush::ts_scl;
    brush_type["ts_shr"] = &brush::ts_shr;
    brush_type["ts_load"] = &brush::ts_load;
    brush_type["clear"] = &brush::clear;
    brush_type["flush"] = &brush::flush;
    brush_type["use_camera"] = &brush::use_camera;
    brush_type["use_program"] = &brush::use_program;
    brush_type["use_state"] = &brush::use_state;
    brush_type["use_blend"] = &brush::use_blend;
    brush_type["viewport"] = &brush::viewport;
    brush_type["scissor"] = &brush::scissor;
    brush_type["scissor_end"] = &brush::scissor_end;
    brush_type["state_"] = &brush::state_;

    // color
    auto color_type = lua_new_usertype<color>(
        _n, "color", lua_constructors<color(), color(double, double, double), color(double, double, double, double)>());
    color_type["r"] = &color::r;
    color_type["g"] = &color::g;
    color_type["b"] = &color::b;
    color_type["a"] = &color::a;
    color_type["__add"] = TYPE_OP_C_(color, color, double, +);
    color_type["__sub"] = TYPE_OP_C_(color, color, double, -);
    color_type["__mul"] = lua_combine(TYPE_OP_C_(color, color, const color&, *), TYPE_OP_C_(color, color, double, *));
    color_type["__div"] = TYPE_OP_C_(color, color, double, /);

    // image
    auto img_type = lua_new_usertype<image>(_n, "image", lua_native);
    img_type["width"] = &image::width;
    img_type["height"] = &image::height;
    img_type["is_from_stb_"] = &image::is_from_stb_;
    img_type["load"] = &image::load;

    // texture
    auto tex_type = lua_new_usertype<texture>(_n, "texture", lua_native);
    tex_type["width"] = &texture::width;
    tex_type["height"] = &texture::height;
    tex_type["texture_id_"] = &texture::texture_id_;
    tex_type["relying_image_"] = &texture::relying_image_;
    tex_type["is_framebuffer_"] = &texture::is_framebuffer_;
    tex_type["full_width"] = &texture::full_width;
    tex_type["full_height"] = &texture::full_width;
    tex_type["u"] = &texture::u;
    tex_type["v"] = &texture::v;
    tex_type["root"] = &texture::root;
    tex_type["make"] = &texture::make;
    tex_type["cut"] = &texture::cut;

    // camera
    auto cam_type = lua_new_usertype<camera>(_n, "camera", lua_constructors<camera()>());
    cam_type["center"] = &camera::center;
    cam_type["view"] = &camera::view;
    cam_type["viewport"] = &camera::viewport;
    cam_type["scale"] = &camera::scale;
    cam_type["rotation"] = &camera::rotation;
    cam_type["combined_t"] = &camera::combined_t;
    cam_type["combined_out_t"] = &camera::combined_out_t;
    cam_type["set_to_static_center"] = &camera::set_to_static_center;
    cam_type["apply"] = &camera::apply;
    cam_type["project"] = &camera::project;
    cam_type["unproject"] = &camera::unproject;
    cam_type["project_x"] = &camera::project_x;
    cam_type["unproject_x"] = &camera::unproject_x;
    cam_type["project_y"] = &camera::project_y;
    cam_type["unproject_y"] = &camera::unproject_y;
    _n["normal"] = &camera::normal;
    _n["gui"] = &camera::gui;
    _n["world"] = &camera::world;

    auto fnt_type = lua_new_usertype<font>(_n, "font", lua_native);
    fnt_type["height"] = &font::height;
    fnt_type["lspc"] = &font::lspc;
    fnt_type["make_vtx"] =
        lua_combine([](font& self, brush* brush, const std::string& u8_str, double x, double y, long align,
                       double max_w, double scale) { return self.make_vtx(brush, u8_str, x, y, align, max_w, scale); },
                    [](font& self, brush* brush, const std::string& u8_str, double x, double y, long align,
                       double max_w) { return self.make_vtx(brush, u8_str, x, y, align, max_w); },
                    [](font& self, brush* brush, const std::string& u8_str, double x, double y, long align) {
                        return self.make_vtx(brush, u8_str, x, y, align);
                    },
                    [](font& self, brush* brush, const std::string& u8_str, double x, double y) {
                        return self.make_vtx(brush, u8_str, x, y);
                    });

    auto fnt_align = lua_make_table();
    fnt_align["left"] = font_align::left;
    fnt_align["right"] = font_align::right;
    fnt_align["up"] = font_align::up;
    fnt_align["down"] = font_align::down;
    fnt_align["hori_center"] = font_align::hori_center;
    fnt_align["vert_center"] = font_align::vert_center;
    fnt_align["normal"] = font_align::normal;
    fnt_align["normal_center"] = font_align::normal_center;

    lua["arc"]["gfx"] = _n;
}

}  // namespace arc