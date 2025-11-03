#pragma once

#include "core/math.h"
#include "core/time.h"
#include "gfx/brush.h"
#include "gfx/font.h"

#define ARC_SIDESCROLLER_SPEED 250
#define ARC_SIDESCROLLER_INF 2
#define ARC_CURSOR_SHINE_INTERVAL 0.6

namespace arc {

struct gui;

struct gui_component {
    quad region;
    std::function<void(quad& region, const quad& view)> locator;
    vec2 cursor_pos;
    int index_in_gui_;
    gui* parent;

    virtual ~gui_component() = default;
    virtual void render(brush* brush) = 0;
    virtual void tick() = 0;
};

struct gui : public std::enable_shared_from_this<gui> {
    static std::vector<std::shared_ptr<gui>> active_guis;
    static void render_currents(brush* brush);
    static void tick_currents();

    std::vector<std::shared_ptr<gui_component>> components_;
    std::function<void(gui& g)> on_closed;
    std::function<void(gui& g)> on_displayed;
    std::shared_ptr<gui_component> focus;

    void join(std::shared_ptr<gui_component> comp);
    void remove(std::shared_ptr<gui_component> comp);
    void clear();
    void display();
    void close();
    bool is_top();

    virtual void render(brush* brush);
    virtual void tick();
};

template <typename T, class... Args>
std::shared_ptr<T> make_gui_component(Args&&... args) {
    static_assert(std::is_base_of<gui_component, T>::value, "T must be a subclass of gui_component.");
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template <typename T, class... Args>
std::shared_ptr<T> make_gui(Args&&... args) {
    static_assert(std::is_base_of<gui, T>::value, "T must be a subclass of gui.");
    return std::make_shared<T>(std::forward<Args>(args)...);
}

struct xside_scroller_ {
    gui_component* c;
    double acc_;
    double bx, by, bw, bh;
    bool is_dragging;
    double lcy_, sp0_;
    double inflation_ = ARC_SIDESCROLLER_INF;
    double prevp_;
    double speed_ = ARC_SIDESCROLLER_SPEED;
    double pos = -ARC_SIDESCROLLER_INF;
    double hsize;
    std::function<void(brush* brush, xside_scroller_* cmp)> on_render;

    void set_speed(double spd);
    void set_inflation(double otl);
    void to_top();
    void to_ground();
    void clamp_();
    void tick();
    void render(brush* brush);
};

enum class button_state { idle, pressed, hovering };

struct gui_button : gui_component {
    std::function<void(brush* brush, gui_button* cmp)> on_render;
    std::function<void()> on_click;
    std::function<void()> on_right_click;
    bool enable_switching = false;
    button_state state = button_state::idle;

    void render(brush* brush) override;
    void tick() override;
};

struct gui_text_view : gui_component {
    bool enable_input = true;
    std::u32string content;
    xside_scroller_ scroller_;
    watch cursor_shiner = watch(ARC_CURSOR_SHINE_INTERVAL, 0.5);
    // the background renderer.
    // the text renderer cannot be customized.
    std::function<void(brush* brush, gui_text_view* cmp)> on_render;
    bool all_selecting;
    int pointer;
    std::shared_ptr<font> font;

    void render(brush* brush) override;
    void tick() override;
};

}  // namespace arc