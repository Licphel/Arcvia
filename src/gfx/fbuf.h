#pragma once

#include "gfx/image.h"

namespace arc {

struct framebuffer : image {
    quad viewport;
    std::shared_ptr<texture> output = nullptr;
    /* unstable */ unsigned int id_, tid_;
    /* unstable */ int g_id_;

    ~framebuffer();

    void retry(brush* brush);
    void record(brush* brush);
    void resize(int w, int h);

    static std::shared_ptr<framebuffer> make();
    static std::shared_ptr<framebuffer> make_fixed(int x, int y);
};

}  // namespace arc