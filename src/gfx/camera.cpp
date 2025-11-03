#include "gfx/camera.h"

#include "core/def.h"
#include "gfx/device.h"

namespace arc {

void camera::apply() {
    double w = view.width / 2;
    double h = view.height / 2;

    projection_t.orthographic(-w, w, -h, h);
    translation_t.identity();

// since the vertices kept pointing upwards, transformations are done here.
// explanation: why don't negate in shader - if do so, we have to translate the
// whole scene by view.h. that's terrible. and this leads to a new problem - the
// matrix passed to gpu differs from we actually use. so the solution is as
// follows - keep two versions of the transformation, rendering using
// #combined_out_t and projection using combined_t.
#ifdef ARC_Y_IS_DOWN
    translation_t.translate(-center.x, -center.y);
    translation_t.rotate(rotation);
    translation_t.scale(scale.x, scale.y);
    combined_t.load(projection_t);
    combined_t.mul(translation_t);
    inverted_t.load(combined_t);
    inverted_t.invert();

    translation_t.identity();
    translation_t.translate(-center.x, center.y);
    translation_t.rotate(rotation);
    translation_t.scale(scale.x, -scale.y);
    combined_out_t.load(projection_t);
    combined_out_t.mul(translation_t);
#else
    translation_t.translate(-center.x, -center.y);
    translation_t.rotate(rotation);
    translation_t.scale(scale.x, scale.y);
    combined_t.load(projection_t);
    combined_t.mul(translation_t);
    inverted_t.load(combined_t);
    inverted_t.invert();
    combined_out_t.load(combined_t);
#endif
}

void camera::set_to_static_center() { center = vec2(view.width / 2.0, view.height / 2.0); }

void camera::project(vec2& v) {
    if (viewport.width <= 0 || viewport.height <= 0) {
        // avoid NaN problem
        return;
    }
    combined_t.apply(v);
    v.x = viewport.width * (v.x + 1) / 2 + viewport.x;
    v.y = viewport.height * (v.y + 1) / 2 + viewport.y;
}

void camera::unproject(vec2& v) {
    if (viewport.width <= 0 || viewport.height <= 0) {
        // avoid NaN problem
        return;
    }
    v.x = 2.0 * (v.x - viewport.x) / viewport.width - 1.0;
    v.y = 2.0 * (v.y - viewport.y) / viewport.height - 1.0;
    inverted_t.apply(v);
}

double camera::project_x(double x) {
    vec2 v = vec2(x, 0);
    project(v);
    return v.x;
}

double camera::project_y(double y) {
    vec2 v = vec2(0, y);
    project(v);
    return v.y;
}

double camera::unproject_x(double x) {
    vec2 v = vec2(x, 0);
    unproject(v);
    return v.x;
}

double camera::unproject_y(double y) {
    vec2 v = vec2(0, y);
    unproject(v);
    return v.y;
}

static camera cam_abs_ = {};
static camera cam_gui_ = {};
static camera cam_world_ = {};

camera& camera::normal() {
    vec2 size = tk_get_size();
    if (size.x == 0 || size.y == 0) return cam_abs_;
    cam_abs_.view = quad(0, 0, size.x, size.y);
    cam_abs_.set_to_static_center();
    cam_abs_.apply();
    cam_abs_.viewport = cam_abs_.view;
    return cam_abs_;
}

camera& camera::gui(bool only_int, double fixed_resolution) {
    vec2 size = tk_get_size();
    if (size.x == 0 || size.y == 0) return cam_gui_;
    double factor = fixed_resolution;
    if (fixed_resolution <= 0) {
        factor = 0.5;
        while (size.x / (factor + 0.5) >= 800.0 && size.y / (factor + 0.5) >= 450.0) factor += 0.5;

        if (only_int && static_cast<int>(factor * 2) % 2 != 0 && factor - 0.5 > 0) factor -= 0.5;
    }
    cam_gui_.view = quad(0, 0, size.x / factor, size.y / factor);
    cam_gui_.set_to_static_center();
    cam_gui_.apply();
    cam_gui_.viewport = quad(0, 0, size.x, size.y);
    return cam_gui_;
}

camera& camera::world(vec2 center, double sight_horizontal) {
    vec2 size = tk_get_size();
    if (size.x == 0 || size.y == 0) return cam_world_;

    double rt = 800.0 / 450.0;
    double fw, fh;

    if (size.x / size.y > rt) {
        fw = size.x;
        fh = size.x / rt;
    } else {
        fh = size.y;
        fw = size.y * rt;
    }

    double x0 = (size.x - fw) / 2, y0 = (size.y - fh) / 2;
    cam_world_.center = center;
    cam_world_.view = {0, 0, sight_horizontal, sight_horizontal / rt};
    cam_world_.apply();
    cam_world_.viewport = {x0, y0, fw, fh};
    return cam_world_;
}

}  // namespace arc