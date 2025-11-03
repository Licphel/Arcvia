#include "gfx/gui.h"

#include <algorithm>

#include "core/chcvt.h"
#include "core/input.h"
#include "core/time.h"
#include "gfx/brush.h"
#include "gfx/camera.h"
#include "gfx/font.h"

namespace arc {

std::vector<std::shared_ptr<gui>> gui::active_guis;

void gui::render_currents(brush* brush) {
    for (auto& g : active_guis) g->render(brush);
}

void gui::tick_currents() {
    for (auto& g : active_guis) g->tick();
}

void gui::join(std::shared_ptr<gui_component> comp) {
    comp->index_in_gui_ = static_cast<int>(components_.size());
    components_.push_back(comp);
    comp->parent = this;
}

void gui::remove(std::shared_ptr<gui_component> comp) {
    int idx = comp->index_in_gui_;
    components_.erase(components_.begin() + idx);
    for (int i = idx; i < static_cast<int>(components_.size()); i++) components_[i]->index_in_gui_ = i;
    comp->index_in_gui_ = -1;
    comp->parent = nullptr;
}

void gui::clear() { components_.clear(); }

void gui::display() {
    active_guis.push_back(shared_from_this());
    if (on_displayed) on_displayed(*this);
}

void gui::close() {
    active_guis.erase(std::remove_if(active_guis.begin(), active_guis.end(),
                                     [this](const std::shared_ptr<gui>& g) { return g.get() == this; }),
                      active_guis.end());
    if (on_closed) on_closed(*this);
}

bool gui::is_top() {
    auto& v = gui::active_guis;
    return v.size() == 0 ? false : v[v.size() - 1].get() == this;
}

void gui::render(brush* brush) {
    auto& cam = camera::gui();
    // do not refer to the old camera - it will change soon.
    auto old_cam = brush->camera_;
    brush->use_camera(cam);
    vec2 cursor_projected = get_cursor();
    // transform cursor to world coordinates
    cam.unproject(cursor_projected);

    for (auto& c : components_) {
        c->cursor_pos = cursor_projected;
        c->render(brush);
    }

    brush->use_camera(old_cam);
}

void gui::tick() {
    auto& cam = camera::gui();

    bool top = is_top();
    if (!top) focus = nullptr;

    for (auto& c : components_) {
        if (c->locator) c->locator(c->region, cam.view);
        c->tick();

        if (top && quad::contain(c->region, c->cursor_pos) && key_press(ARC_MOUSE_BUTTON_LEFT)) focus = c;
    }
}

// xside_scroller

void xside_scroller_::set_speed(double spd) { speed_ = spd; }

void xside_scroller_::set_inflation(double otl) {
    inflation_ = otl;
    pos = -inflation_;
}

void xside_scroller_::to_top() {
    pos = -inflation_;
    clamp_();
    prevp_ = pos;
}

void xside_scroller_::to_ground() {
    pos = hsize - c->region.height + inflation_ * 2;
    clamp_();
    prevp_ = pos;
}

void xside_scroller_::clamp_() {
    // A minimum offset.
    if (pos <= -inflation_) {
        pos = -inflation_;
        acc_ = 0;
    }

    if (pos - hsize - inflation_ >= -c->region.height) {
        pos = hsize - c->region.height + inflation_;
        acc_ = 0;
    }

    if (hsize + inflation_ * 2 < c->region.height) {
        pos = -inflation_;
        acc_ = 0;
    }
}

void xside_scroller_::tick() {
    int scrd = get_scroll_towards();
    double scr = consume_scroll();

    prevp_ = pos;

    if (quad::contain(c->region, c->cursor_pos) && scr != 0) {
        switch (scrd) {
            case ARC_SCROLL_UP:
                acc_ -= speed_;
                break;
            case ARC_SCROLL_DOWN:
                acc_ += speed_;
                break;
        }
    }

    pos += acc_ * clock::now().delta;
    if (acc_ > 0)
        acc_ = std::clamp(acc_ - clock::now().delta * speed_ * 6, 0.0, static_cast<double>(INT_MAX));
    else if (acc_ < 0)
        acc_ = std::clamp(acc_ + clock::now().delta * speed_ * 6, static_cast<double>(INT_MIN), 0.0);
    clamp_();

    if (key_held(ARC_MOUSE_BUTTON_LEFT)) {
        if (!is_dragging) {
            double mx = c->cursor_pos.x;
            double my = c->cursor_pos.y;

            if (mx >= bx - 1 && mx <= bx + bw + 1 && my >= by - 1 && my <= by + bh + 1) {
                is_dragging = true;
                lcy_ = c->cursor_pos.y;
                sp0_ = pos;
            }
        }
    } else {
        is_dragging = false;
    }

    if (is_dragging) {
        pos = sp0_ - (lcy_ - c->cursor_pos.y) / c->region.height * hsize;
        clamp_();
    }
}

void xside_scroller_::render(brush* brush) {
    double per = (c->region.height - inflation_ * 2) / hsize;
    if (per > 1) per = 1;

    double sper = abs(lerp(prevp_, pos)) / hsize;
    double h = (c->region.height - inflation_ * 2) * per;
    double oh = sper * c->region.height;

    bh = h;
    bx = c->region.prom_x() - 4 - inflation_;
    by = c->region.y + oh + inflation_;
    bw = 4;

    if (per < 1) {
        if (on_render)
            on_render(brush, this);
        else
            brush->draw_rect(quad(bx, by, bw, bh));
    }
}

// gui_button

void gui_button::render(brush* brush) { on_render(brush, this); }

void gui_button::tick() {
    bool hovering = quad::contain(region, cursor_pos);
    bool ld = key_press(ARC_MOUSE_BUTTON_LEFT);
    bool rd = key_press(ARC_MOUSE_BUTTON_RIGHT);

    if (hovering) {
        if (ld && on_click) on_click();
        if (rd && on_right_click) on_right_click();
    }

    if (enable_switching) {
        if (hovering && (ld || rd))
            state = (state == button_state::pressed) ? (hovering ? button_state::hovering : button_state::idle)
                                                     : button_state::pressed;
    } else {
        state = button_state::idle;
        if (hovering) {
            state = button_state::hovering;
            if (ld) state = button_state::pressed;
            if (rd) state = button_state::pressed;
        }
    }
}

// gui_text_view

void gui_text_view::render(brush* brush) {
    scroller_.c = this;
    if (on_render) on_render(brush, this);

    if (content.length() >= 32767) return;

    double o = scroller_.inflation_;
    double x = region.x + o;
    double y = region.y + o;

    double pos = lerp(scroller_.prevp_, scroller_.pos);

    brush->scissor(region);
    font_render_bound bd = font->make_vtx(brush, content, region.x + o, region.y - pos, font_align::normal,
                                          region.width - o * 3 - scroller_.bw);
    brush->scissor_end();
    scroller_.hsize = bd.region.height;
    scroller_.render(brush);

    if (parent->focus.get() == this && !cursor_shiner) {
        bd = font->make_vtx(nullptr, content.substr(0, pointer), region.x + o, region.y - pos, font_align::normal,
                            region.width - o * 3 - scroller_.bw);
        x += bd.last_width;

        if (content.length() > 0) {
            auto lglyph = font->get_glyph(content[content.length() - 1]);
            x += lglyph.advance - lglyph.size.x;
        }

        y += pos + (bd.lines == 0 ? 0 : (bd.lines - 1) * font->lspc);
        brush->scissor(region);
        brush->cl_set(color(1, 1, 1, 0.6));
        brush->draw_rect(quad(x, y + 1, 1, font->lspc));
        brush->cl_norm();
        brush->scissor_end();
    }
}

static void insert_(std::u32string& str, int& pointer, const std::string ins) {
    static std::u32string cvtbuf_;
    cvt_u32_(ins, &cvtbuf_);

    if (str.length() >= 32767) return;
    if (pointer == static_cast<int>(str.size()))
        str += cvtbuf_;
    else
        str.insert(pointer, cvtbuf_);
    pointer += cvtbuf_.length();
}

static void on_edit_(gui_text_view* c) {
    if (c->all_selecting) {
        c->content.clear();
        c->pointer = 0;
    }
    c->all_selecting = false;
}

void gui_text_view::tick() {
    static std::string cvtbuf_;

    scroller_.c = this;
    scroller_.tick();

    if (parent->focus.get() != this) return;

    std::string txt = consume_chars();
    if (txt.length() > 0) insert_(content, pointer, txt);

    if (key_press(ARC_KEY_V, ARC_MOD_CONTROL)) {
        on_edit_(this);
        insert_(content, pointer, consume_clipboard_text());
    }

    if (key_press(ARC_KEY_A, ARC_MOD_CONTROL)) all_selecting = !all_selecting;

    if (key_press(ARC_KEY_C, ARC_MOD_CONTROL) && all_selecting) {
        cvt_u8_(content, &cvtbuf_);
        set_clipboard_text(cvtbuf_);
        all_selecting = false;
    }

    if (key_press(ARC_KEY_X, ARC_MOD_CONTROL) && all_selecting) {
        cvt_u8_(content, &cvtbuf_);
        set_clipboard_text(cvtbuf_);
        pointer = 0;
        content.clear();
        all_selecting = false;
    }

    if (key_press(ARC_KEY_ENTER) || key_repeat(ARC_KEY_ENTER)) {
        on_edit_(this);
        insert_(content, pointer, "\n");
    }

    if (key_press(ARC_KEY_LEFT) || key_repeat(ARC_KEY_LEFT)) pointer = std::max(0, pointer - 1);
    if (key_press(ARC_KEY_RIGHT) || key_repeat(ARC_KEY_RIGHT))
        pointer = std::min(static_cast<int>(content.length()), pointer + 1);

    if (key_press(ARC_KEY_BACKSPACE) || key_repeat(ARC_KEY_BACKSPACE)) {
        if (content.length() > 0 && pointer != 0) {
            content.erase(pointer - 1, 1);
            pointer = std::max(0, pointer - 1);
        }

        if (all_selecting) {
            pointer = 0;
            content.clear();
            all_selecting = false;
        }
    }
}

}  // namespace arc