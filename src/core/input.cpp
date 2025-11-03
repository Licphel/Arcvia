#include "core/input.h"

#include "gfx/device.h"

namespace arc {

bool key_held(int key, int mod) { return tk_key_held_(key, mod); }
bool key_press(int key, int mod) { return tk_key_press_(key, mod); }
bool key_repeat(int key, int mod) { return tk_key_repeat_(key, mod); }
vec2 get_cursor() { return tk_get_cursor_(); }
double consume_scroll() { return tk_consume_scroll_(); }
int get_scroll_towards() { return tk_get_scroll_towards_(); }
std::string consume_chars() { return tk_consume_chars_(); }

std::string consume_clipboard_text() { return tk_consume_clipboard_text_(); }

void set_clipboard_text(const std::string& str) { tk_set_clipboard_text_(str); }

}  // namespace arc
