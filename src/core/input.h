#pragma once
#include <string>

#include "core/key.h"
#include "core/math.h"

// this header is a single wrapper of #gfx/device.h
// it prevents native libraries from leaking into your codes.
namespace arc {

bool key_held(int key, int mod = ARC_MOD_ANY);
bool key_press(int key, int mod = ARC_MOD_ANY);
// check if a key is pressed or repeated.
bool key_repeat(int key, int mod = ARC_MOD_ANY);
vec2 get_cursor();
// consumes an abs value of scroll.
double consume_scroll();
// returns ARC_SCROLL_UP, ARC_SCROLL_DOWN or ARC_SCROLL_NO
int get_scroll_towards();
// consume the typed characters since last call.
std::string consume_chars();
std::string consume_clipboard_text();
void set_clipboard_text(const std::string& str);

}  // namespace arc