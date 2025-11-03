#include "gfx/npatch.h"

#include "gfx/brush.h"
#include "gfx/image.h"

namespace arc {

nine_patches::nine_patches() = default;

nine_patches::nine_patches(std::shared_ptr<texture> tex) : nine_patches(tex, 1) {}

nine_patches::nine_patches(std::shared_ptr<texture> tex, double scale) : scale(scale) {
    double p13 = 1.0 / 3.0 * tex->width;
    double p23 = 2.0 / 3.0 * tex->height;
    tw = p13 * scale;
    th = p13 * scale;

    lt = tex->cut(quad(0, 0, p13, p13));
    t = tex->cut(quad(p13, 0, p13, p13));
    rt = tex->cut(quad(p23, 0, p13, p13));
    l = tex->cut(quad(0, p13, p13, p13));
    c = tex->cut(quad(p13, p13, p13, p13));
    r = tex->cut(quad(p23, p13, p13, p13));
    lb = tex->cut(quad(0, p23, p13, p13));
    b = tex->cut(quad(p13, p23, p13, p13));
    rb = tex->cut(quad(p23, p23, p13, p13));

#ifndef ARC_Y_IS_DOWN
    std::swap(lt, lb);
    std::swap(rt, rb);
    std::swap(t, b);
#endif
}

void nine_patches::make_vtx(brush* brush, const quad& dst) const {
    double nw = ceil(dst.width / tw);
    double nh = ceil(dst.height / tw);
    double rw = fmod(dst.width, tw);
    if (rw == 0) rw = tw;
    double rh = fmod(dst.height, th);
    if (rh == 0) rh = th;
    double x = dst.x, y = dst.y;
    double x2 = x + (enable_overlapping_ ? dst.width - tw : tw * (nw - 1));
    double y2 = y + (enable_overlapping_ ? dst.height - th : th * (nh - 1));

    for (int i = 1; i < nw - 1; i++)
        for (int j = 1; j < nh - 1; j++) {
            double w1 = i == nw - 2 ? rw : tw;
            double h1 = j == nh - 2 ? rh : th;
            brush->draw_texture(c, quad(x + i * tw, y + j * th, w1, h1), quad(c->width - w1, 0, w1, h1));
        }

    brush->draw_texture(lt, quad(x, y, tw, th));

    for (int i = 1; i < nh - 1; i++) {
        double h1 = i == nh - 2 ? rh : th;
        brush->draw_texture(l, quad(x, y + i * th, tw, h1), quad(0, 0, tw, h1));
    }

    for (int i = 1; i < nw - 1; i++) {
        double w1 = i == nw - 2 ? rw : tw;
        brush->draw_texture(t, quad(x + i * tw, y, w1, th), quad(b->width - w1, 0, w1, th));
    }

    brush->draw_texture(rt, quad(x2, y, tw, th));

    for (int i = 1; i < nh - 1; i++) {
        double h1 = i == nh - 2 ? rh : th;
        brush->draw_texture(r, quad(x2, y + i * th, tw, h1), quad(0, 0, tw, h1));
    }

    brush->draw_texture(lb, quad(x, y2, tw, th));

    for (int i = 1; i < nw - 1; i++) {
        double w1 = i == nw - 2 ? rw : tw;
        brush->draw_texture(b, quad(x + i * tw, y2, w1, th), quad(t->width - w1, 0, w1, th));
    }

    brush->draw_texture(rb, quad(x2, y2, tw, th));
}

}  // namespace arc