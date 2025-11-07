#include "gfx/fbuf.h"

#include <memory>

#include "core/log.h"
#include "gfx/device.h"
#include "gfx/image.h"

// clang-format off
#include <gl/glew.h>
#include <gl/gl.h>
// clang-format on

namespace arc {

static int fb_idic = 0;

framebuffer::~framebuffer() { glDeleteFramebuffers(1, &id_); }

void framebuffer::retry(brush* brush) {
    viewport = brush->camera_.viewport;
    brush->flush();
    glBindFramebuffer(GL_FRAMEBUFFER, id_);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void framebuffer::record(brush* brush) {
    brush->flush();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void framebuffer::resize(int w, int h) {
    if (w == 0 || h == 0) return;
    glBindFramebuffer(GL_FRAMEBUFFER, id_);
    glBindTexture(GL_TEXTURE_2D, tid_);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (output == nullptr) output = std::make_shared<texture>();
    output->texture_id_ = tid_;
    output->is_framebuffer_ = true;
    output->width = output->full_width = w;
    output->height = output->full_height = h;
    output->parameters({.uv = texture_parameter::uv_clamp,
                        .min_filter = texture_parameter::filter_nearest,
                        .mag_filter = texture_parameter::filter_nearest});
    // pit-fall: in #parameters, texture is binded to 0
    glBindTexture(GL_TEXTURE_2D, tid_);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tid_, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        print_throw(log_level::fatal, "cannot make framebuffer: status = {}.", status);
    }
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        print_throw(log_level::fatal, "GL error in framebuffer resize: {}", err);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    viewport = quad(0, 0, w, h);
}

std::shared_ptr<framebuffer> framebuffer::make() {
    auto ptr = std::make_shared<framebuffer>();
    ptr->g_id_ = fb_idic++;
    glGenFramebuffers(1, &ptr->id_);
    glGenTextures(1, &ptr->tid_);
    vec2 s = tk_get_size();
    ptr->resize(static_cast<int>(s.x), static_cast<int>(s.y));
    event_resize += [weak = std::weak_ptr<framebuffer>(ptr)](int w, int h) {
        if (auto ptr_u = weak.lock()) {
            ptr_u->resize(w, h);
        }
    };
    return ptr;
}

std::shared_ptr<framebuffer> framebuffer::make_fixed(int x, int y) {
    auto ptr = std::make_shared<framebuffer>();
    ptr->g_id_ = fb_idic++;
    glGenFramebuffers(1, &ptr->id_);
    glGenTextures(1, &ptr->tid_);
    ptr->resize(static_cast<int>(x), static_cast<int>(y));
    return ptr;
}

}  // namespace arc