#pragma once
#include <string>

#include "core/delg.h"
#include "core/math.h"
#include "gfx/brush.h"
#include "gfx/image.h"

namespace arc {

struct brush;

inline delegate<void()> event_tick;
inline delegate<void(brush* brush)> event_render;
inline delegate<void()> event_dispose;
inline delegate<void(int w, int h)> event_resize;
inline delegate<void(int button, int action, int mods)> event_mouse_state;
inline delegate<void(double x, double y)> event_cursor_pos;
inline delegate<void(double x, double y)> event_mouse_scroll;
inline delegate<void(int button, int scancode, int action, int mods)> event_key_state;

int tk_real_fps();
int tk_real_tps();

void tk_make_handle();
void tk_title(const std::string& title);
void tk_size(vec2 size);
void tk_pos(vec2 pos);
void tk_visible(bool visible);
void tk_maximize();
void tk_icon(std::shared_ptr<image> img);
void tk_cursor(std::shared_ptr<image> img, vec2 hotspot);
void tk_end_make_handle();
std::string tk_get_title();
vec2 tk_get_size();
vec2 tk_get_pos();
vec2 tk_get_device_size();

// begins the main loop (blocks the current thread).
// fps, if negative, means uncapped.
void tk_lifecycle(int fps, int tps, bool vsync);

void tk_swap_buffers();
bool tk_poll_events();

bool tk_key_held_(int key, int mod);
bool tk_key_press_(int key, int mod);
bool tk_key_repeat_(int key, int mod);
vec2 tk_get_cursor_();
// consumes an abs value of scroll.
double tk_consume_scroll_();
// returns ARC_SCROLL_UP, ARC_SCROLL_DOWN or ARC_SCROLL_NO
int tk_get_scroll_towards_();
// consume the typed characters since last call.
std::string tk_consume_chars_();
std::string tk_consume_clipboard_text_();
void tk_set_clipboard_text_(const std::string& str);

}  // namespace arc
