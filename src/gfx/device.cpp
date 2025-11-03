#include "gfx/device.h"

#include <algorithm>
#include <string>

#include "core/def.h"
#include "core/key.h"
#include "core/log.h"
#include "core/time.h"
#include "gfx/image.h"
#include "gfx/mesh.h"

// clang-format off
#include <gl/glew.h>
#include <gl/gl.h>
#include <glfw/glfw3.h>
// clang-format on

namespace arc {

static GLFWwindow* window;

static long keydown[512];
static long keydown_render[512];
static char keymod[512];
static char keyact[512];
static double mcx, mcy;
static double mscx, mscy;
static int rfps, rtps;
static bool cur_in_tick_;
static std::string char_seq_;
static bool just_ticked_;

static std::shared_ptr<mesh> direct_mesh = nullptr;

double nanos_() { return glfwGetTime() * 1'000'000'000; }

int tk_real_fps() { return rfps; }

int tk_real_tps() { return rtps; }

void tk_make_handle() {
    if (!glfwInit()) print_throw(log_level::fatal, "glfw cannot initialize.");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

    window = glfwCreateWindow(128, 128, "", NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        print_throw(log_level::fatal, "glfw make window failed.");
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) print_throw(log_level::fatal, "glew cannot initialize.");

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int nw, int nh) { event_resize(nw, nh); });
    glfwSetMouseButtonCallback(window, [](GLFWwindow*, int button, int action, int mods) {
        button += 480; /* **magic number** ARC_MOUSE_BUTTON_BEGIN */
        event_mouse_state(button, action, mods);
        keydown[button] = clock::now().ticks;
        keydown_render[button] = clock::now().render_ticks;
        keyact[button] = action;
        keymod[button] = mods;
    });
    glfwSetScrollCallback(window, [](GLFWwindow*, double x, double y) {
        event_mouse_scroll(x, y);
        mscx = x;
        mscy = y;
    });
    glfwSetCursorPosCallback(window, [](GLFWwindow*, double x, double y) {
        event_cursor_pos(x, y);
        mcx = x;
#ifdef ARC_Y_IS_DOWN
        mcy = y;
#else
        mcy = tk_get_size().y - y;
#endif
    });
    glfwSetKeyCallback(window, [](GLFWwindow*, int button, int scancode, int action, int mods) {
        event_key_state(button, scancode, action, mods);
        keydown[button] = clock::now().ticks;
        keydown_render[button] = clock::now().render_ticks;
        keyact[button] = action;
        keymod[button] = mods;
    });
    glfwSetCharCallback(window, [](GLFWwindow*, unsigned int cp) {
        if (cp <= 0x7F) {
            char_seq_.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            char_seq_.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            char_seq_.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            char_seq_.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            char_seq_.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            char_seq_.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0x10FFFF) {
            char_seq_.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            char_seq_.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            char_seq_.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            char_seq_.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    });

    // set opengl caps
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(
        [](GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar* txt, const void*) { 
            print(log_level::info, "GL: {}", txt); 
        },
        nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
}

void tk_title(const std::string& title) { glfwSetWindowTitle(window, title.c_str()); }

void tk_size(vec2 size) {
    glfwSetWindowSize(window, size.x, size.y);
    vec2 dsize = tk_get_device_size();
    tk_pos((dsize - size) / 2);
}

void tk_pos(vec2 pos) { glfwSetWindowPos(window, pos.x, pos.y); }

vec2 tk_get_device_size() {
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* vidm = glfwGetVideoMode(monitor);
    return vec2(vidm->width, vidm->height);
}

void tk_visible(bool visible) {
    if (visible)
        glfwShowWindow(window);
    else
        glfwHideWindow(window);
}

void tk_maximize() { glfwMaximizeWindow(window); }

void tk_icon(std::shared_ptr<image> img) {
    GLFWimage img_glfw;
    img_glfw.pixels = img->pixels;
    img_glfw.width = img->width;
    img_glfw.height = img->height;
    glfwSetWindowIcon(window, 1, &img_glfw);
}

void tk_cursor(std::shared_ptr<image> img, vec2 hotspot) {
    GLFWimage img_glfw;
    img_glfw.pixels = img->pixels;
    img_glfw.width = img->width;
    img_glfw.height = img->height;
    GLFWcursor* cursor = glfwCreateCursor(&img_glfw, hotspot.x, hotspot.y);
    glfwSetCursor(window, cursor);
}

void tk_end_make_handle() { glfwShowWindow(window); }

std::string tk_get_title() { return glfwGetWindowTitle(window); }

vec2 tk_get_size() {
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    return vec2(w, h);
}

vec2 tk_get_pos() {
    int x, y;
    glfwGetWindowPos(window, &x, &y);
    return vec2(x, y);
}

void tk_lifecycle(int fps, int tps, bool vsync) {
    // init global gfx usage
    direct_mesh = mesh::make();
    direct_mesh->is_direct_ = true;
    // end region
    glfwSwapInterval(vsync ? 1 : 0);

    const double DT_LOGIC_NS = 1'000'000'000.0 / tps;
    const double DT_RENDER_NS = (fps > 0) ? 1'000'000'000.0 / fps : 0.0;

    double current = nanos_();
    double logic_debt = 0.0;
    double last_render = current;

    rfps = fps;
    rtps = tps;
    clock::now().delta = DT_LOGIC_NS / 1'000'000'000.0;

    int tick_frm = 0, render_frm = 0;
    double last_calc = current;
    double last_stat = current;

    try {
        while (!tk_poll_events()) {
            current = nanos_();

            logic_debt += (current - last_calc);
            last_calc = current;

            int max_catch = 4;
            just_ticked_ = false;

            while (logic_debt >= DT_LOGIC_NS && max_catch--) {
                cur_in_tick_ = true;
                just_ticked_ = true;
                event_tick();
                cur_in_tick_ = false;
                clock::now().ticks++;
                clock::now().seconds += clock::now().delta;
                tick_frm++;
                logic_debt -= DT_LOGIC_NS;
            }

            if (fps <= 0 || current - last_render >= DT_RENDER_NS) {
                clock::now().partial = std::clamp(1.0 - (logic_debt / DT_LOGIC_NS), 0.0, 1.0);

                auto* brush = direct_mesh->brush_.get();
                event_render(brush);
                clock::now().render_ticks++;
                brush->flush();
                tk_swap_buffers();

                render_frm++;
                last_render += DT_RENDER_NS;
                if (last_render < current - DT_RENDER_NS) last_render = current;
            }

            if (current - last_stat > 500'000'000.0) {
                rtps = tick_frm * 2;
                rfps = render_frm * 2;
                tick_frm = render_frm = 0;
                last_stat = current;
            }
        }
    } catch (std::exception& e) {
        print(log_level::fatal, "fatal error occurred: {}", e.what());
        // re-throw for debugging & stack trace.
        throw e;
    }

    event_dispose();
}

void tk_swap_buffers() { glfwSwapBuffers(window); }

bool tk_poll_events() {
    glfwPollEvents();

    bool v = glfwWindowShouldClose(window);
    if (v) glfwTerminate();
    return v;
}

bool tk_key_held_(int key, int mod) { return keyact[key] != GLFW_RELEASE && (mod == ARC_MOD_ANY || keymod[key] & mod); }

bool tk_key_press_(int key, int mod) {
    bool pressc = (cur_in_tick_ && keydown[key] == clock::now().ticks) ||
                  (!cur_in_tick_ && keydown_render[key] == clock::now().render_ticks);
    return keyact[key] == GLFW_PRESS && pressc && (mod == ARC_MOD_ANY || keymod[key] & mod);
}

bool tk_key_repeat_(int key, int mod) {
    return keyact[key] == GLFW_REPEAT && (mod == ARC_MOD_ANY || keymod[key] & mod) && just_ticked_;
}

vec2 tk_get_cursor_() { return vec2(mcx, mcy); }

double tk_consume_scroll_() {
    double d = std::abs(mscy);
    mscy = 0;
    return d;
}

int tk_get_scroll_towards_() {
    if (mscy > 10E-4) return ARC_SCROLL_UP;
    if (mscy < -10E-4) return ARC_SCROLL_DOWN;
    return ARC_SCROLL_NO;
}

std::string tk_consume_chars_() {
    std::string cpy = char_seq_;
    char_seq_.clear();
    return cpy;
}

std::string tk_consume_clipboard_text_() {
    const char* str = glfwGetClipboardString(window);
    return str == nullptr ? "" : std::string(str);
}

void tk_set_clipboard_text_(const std::string& str) { glfwSetClipboardString(window, str.c_str()); }

}  // namespace arc
