#include <memory>
#include <string>

#include "audio/device.h"
#include "core/buffer.h"
#include "core/input.h"
#include "core/io.h"
#include "core/key.h"
#include "core/load.h"
#include "core/log.h"
#include "core/rand.h"
#include "ctt.h"
#include "gfx/atlas.h"
#include "gfx/brush.h"
#include "gfx/camera.h"
#include "gfx/device.h"
#include "gfx/font.h"
#include "gfx/gui.h"
#include "gfx/image.h"
#include "gfx/mesh.h"
#include "gfx/npatch.h"
#include "lua/lua.h"
#include "net/packet.h"
#include "net/socket.h"
#include "render/block_model.h"
#include "render/chunk_model.h"
#include "render/light.h"
#include "render/liquid_model.h"
#include "render/world_model.h"
#include "world/block.h"
#include "world/chunk.h"
#include "world/dim.h"
#include "world/entity.h"
#include "world/liquid.h"

using namespace arc;

int i;
socket& sockc = *socket::remote;
socket& socks = *socket::server;
std::shared_ptr<gui> g;
std::shared_ptr<gui_button> b;
std::shared_ptr<gui_text_view> tv;
std::shared_ptr<font> fnt;
nine_patches pct;
dimension* dim;
std::shared_ptr<atlas> atl;

struct posic {
    double x, y;

    void write(byte_buf& buf) {
        buf.write<double>(x);
        buf.write<double>(y);
    }

    void read(byte_buf& buf) {
        x = buf.read<double>();
        y = buf.read<double>();
    }
};

int main() {
    tk_make_handle();
    tk_title("arcvia test");
    tk_size(vec2(800, 450));
    tk_icon(image::load(path::open_local("gfx/misc/logo.png")));
    tk_end_make_handle();

    R_make_nonnulls();
    atl = atlas::make(1024, 1024);
    atl->begin();
    block_behavior beh = block_behavior();
    beh.shape = block_shape::opaque;
    auto DIRT_MODEL = R_block_models().make(
        "arc:dirt", block_model{.dropper = block_dropper::repeat,
                                .dynamic_render = false,
                                .tex = atl->accept(image::load(path::open_local("gfx/dirt.png")))});
    auto DIRT = R_blocks().make("arc:dirt", beh);
    DIRT->model = DIRT_MODEL;
    beh = block_behavior();
    beh.shape = block_shape::opaque;
    auto ROCK_MODEL = R_block_models().make(
        "arc:rock", block_model{.dropper = block_dropper::repeat,
                                .dynamic_render = false,
                                .tex = atl->accept(image::load(path::open_local("gfx/rock.png")))});
    auto ROCK = R_blocks().make("arc:rock", beh);
    ROCK->model = ROCK_MODEL;

    liquid_behavior lbeh = liquid_behavior();
    lbeh.cast_light = [](dimension* dim, const pos2i& pos, liquid_stack& s, int pipe) {
        if (pipe == 0) return 0.8;
        if (pipe == 1) return 0.67;
        return 0.35;
    };
    lbeh.density = 1000;
    lbeh.stickiness = 3000;
    auto LAVA = R_liquids().make("lava", lbeh);
    auto LAVA_MODEL = R_liquid_models().make(
        "lava", liquid_model{.tex = texture::make(image::load(path::open_local("gfx/lava.png"))),
                             .tex_edge = texture::make(image::load(path::open_local("gfx/lava_edge.png")))});
    LAVA->model = LAVA_MODEL;

    R_blocks().work();
    R_block_models().work();
    R_items().work();
    R_block_models().work();
    R_liquids().work();
    R_liquid_models().work();
    atl->end();

    dim = new dimension();
    auto chunk_ptr = std::make_shared<chunk>();
    auto chunk_ptr1 = std::make_shared<chunk>();
    chunk_ptr->init(dim, {0, 0});
    chunk_ptr1->init(dim, {0, 1});
    dim->set_chunk({0, 0}, chunk_ptr);
    dim->set_chunk({0, 1}, chunk_ptr1);
    dim->init();
    for (int x = 0; x < 16; x++) {
        for (int y = 3; y < 16; y++) {
            dim->set_block(random::rg->next_bool() ? block_void : ROCK, {x, y});
            dim->set_back_block(random::rg->next_bool() ? DIRT : ROCK, {x, y});
        }
    }
    for (int x = 0; x < 16; x++) {
        for (int y = 16; y < 32; y++) {
            dim->set_block(ROCK, {x, y});
        }
    }

    fnt = font::load(path::open_local("gfx/font/main.ttf"), 12, font_style::regular);
    auto tex = texture::make(image::load(path::open_local("gfx/misc/logo.png")));
    pct = nine_patches(tex);
    get_resource_map_()["arc:gfx/misc/logo.png"] = tex;

    lua_make_state();
    lua_bind_modules();
    lua_eval(io::read_str(path::open_local("main.lua")));
    packet::mark_id<packet_2s_heartbeat>();
    packet::mark_id<packet_dummy>();

    g = make_gui<gui>();

    b = make_gui_component<gui_button>();
    b->locator = [](quad& region, const quad& view) {
        region = quad::center(view.center_x(), view.center_y(), 200, 40);
    };
    b->on_render = [](brush* brush, gui_button* cmp) {
        if (cmp->state == button_state::idle)
            brush->cl_set(color(1, 1, 1, 1));
        else if (cmp->state == button_state::hovering)
            brush->cl_set(color(0.8, 0.8, 1, 1));
        else if (cmp->state == button_state::pressed)
            brush->cl_set(color(0.6, 0.6, 1, 1));
        pct.make_vtx(brush, b->region);
        brush->cl_norm();
    };
    b->on_click = []() { print(log_level::info, "I'm clicked!"); };
    g->join(b);

    tv = make_gui_component<gui_text_view>();
    tv->font = fnt;
    tv->locator = [](quad& region, const quad& view) {
        region = quad::center(view.center_x() - 250, view.center_y(), 300, 120);
    };
    tv->on_render = [](brush* brush, gui_text_view* cmp) { brush->draw_rect_outline(cmp->region); };
    g->join(tv);

    g->display();
    std::shared_ptr<entity> e_0 = std::make_shared<entity>();
    e_0->box = quad::center(2.5, 0, 1.75, 2.75);
    e_0->mass = 60;
    e_0->cat = entity_cat::creature;
    dim->spawn_entity(e_0);

    uint16_t port = gen_tcp_port_();
    socks.start(port);
    sockc.connect(connection_type::lan);

    event_tick += [&]() {
        dim->tick_systems();
        // lua_protected_call(lua_get<lua_function>("tick"), dim);
        // sockc.send_to_server(packet::make<packet_dummy>("hello world"));
        sockc.tick();
        socks.tick();
        bool b = random::rg->next_bool();
        dim->set_block(b ? DIRT : ROCK, {2, 2});
        dim->tick();
        dim->light_executor->tick(quad::center(10, 10, 60, 45));

        const double vi = 20.0;
        if (key_held(ARC_KEY_D)) e_0->move({vi * clock::now().delta, 0}, {3, 0}, physic_ignore::land_f);
        if (key_held(ARC_KEY_A)) e_0->move({-vi * clock::now().delta, 0}, {3, 0}, physic_ignore::land_f);
        if (key_held(ARC_KEY_SPACE) && (e_0->phy_status & phybit::touch_d)) {
            e_0->impulse({0, -5.0 * 60});
        }
        if (key_held(ARC_KEY_SPACE) && (e_0->phy_status & phybit::swim)) {
            e_0->impulse({0, -7.5 * 60 * clock::now().delta});
        }

        camera& c = camera::world({10, 10}, 60);
        vec2 curs = get_cursor();
        c.unproject(curs);

        if (key_held(ARC_MOUSE_BUTTON_LEFT)) dim->set_block(block_void, {curs.x, curs.y});
        if (key_held(ARC_MOUSE_BUTTON_RIGHT, ARC_MOD_CONTROL))
            dim->set_liquid_stack({LAVA, liquid_stack::max_amount}, {curs.x, curs.y});
        else if (key_held(ARC_MOUSE_BUTTON_RIGHT))
            dim->set_block(ROCK, {curs.x, curs.y});
    };

    event_render += [&](brush* brush) {
        brush->clear({0, 0, 0, 1});
        brush->use_camera(camera::normal());
        brush->use_blend(blend_mode::normal);
        gui::tick_currents();
        // gui::render_currents(brush);

        brush->use_camera(camera::gui());
        fnt->make_vtx(brush, "fps: " + std::to_string(tk_real_fps()), 15, 15);
        fnt->make_vtx(brush, "velo: " + std::to_string(e_0->velocity.x), 15, 30);
        // lua_protected_call(lua_get<lua_function>("draw"), brush);

        brush->use_camera(camera::world({10, 10}, 60));
        reflush_chunk_models_(dim, {0, 0});

        wrd::retry();
        wrd::render(brush, dim, brush->camera_.view);
        wrd::submit(brush, dim);
        brush->draw_rect_outline({0, 0, 1, 1});
    };

    tk_make_device();
    tk_set_device_option(device_option::roll_off, 2.0);
    tk_set_device_option(device_option::ref_dist, 8.0);
    tk_set_device_option(device_option::max_dist, 42.0);
    tk_set_device_option(device_option::listener, vec3(0, 0, 0));
    tk_end_make_device();

    tk_lifecycle(0, 20, false);

    sockc.disconnect();
    socks.stop();
    lua_free();

    delete dim;
}
