#include "gfx/image.h"

#include "core/io.h"
#include "core/log.h"

// clang-format off
#include <gl/glew.h>
#include <gl/gl.h>
// clang-format on

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace arc {

image::image() = default;

image::image(uint8_t* data, int w, int h) : width(w), height(h), pixels(data) {}

image::~image() {
    if (is_from_stb_)
        stbi_image_free(pixels);
    else
        delete[] pixels;
    pixels = nullptr;
}

std::shared_ptr<image> image::load(const path& path_) {
    std::shared_ptr<image> img = std::make_shared<image>();
    img->pixels = stbi_load(path_.strp.c_str(), &img->width, &img->height, nullptr, 4);
    if (img->pixels == nullptr) print_throw(log_level::fatal, "path not found: {}", path_.strp);
    img->is_from_stb_ = true;
    return img;
}

std::shared_ptr<image> image::make(int width, int height, uint8_t* data) {
    std::shared_ptr<image> img = std::make_shared<image>();
    img->width = width;
    img->height = height;
    img->pixels = data;
    img->is_from_stb_ = false;
    return img;
}

texture::~texture() {
    if (root == nullptr) glDeleteTextures(1, &texture_id_);
}

std::shared_ptr<texture> texture::make(std::shared_ptr<image> img) {
    std::shared_ptr<texture> tex = std::make_shared<texture>();
    unsigned int id;
    glGenTextures(1, &id);
    tex->texture_id_ = id;

    if (img != nullptr) tex->link_data_(img);

    return tex;
}

void texture::parameters(texture_parameters param) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id_);

    GLenum mode0;
    switch (param.uv) {
        case texture_parameter::uv_clamp:
            mode0 = GL_CLAMP_TO_EDGE;
            break;
        case texture_parameter::uv_mirror:
        default:
            mode0 = GL_MIRRORED_REPEAT;
            break;
        case texture_parameter::uv_repeat:
            mode0 = GL_REPEAT;
            break;
    }
    GLenum mode1 = param.min_filter == texture_parameter::filter_linear ? GL_LINEAR : GL_NEAREST;
    GLenum mode2 = param.mag_filter == texture_parameter::filter_linear ? GL_LINEAR : GL_NEAREST;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mode1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mode2);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void texture::link_data_(std::shared_ptr<image> img) {
    relying_image_ = img;
    full_width = width = img->width;
    full_height = height = img->height;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img->width, img->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);

    parameters({});

    glBindTexture(GL_TEXTURE_2D, 0);
}

std::shared_ptr<texture> texture::cut(const quad& src) {
    std::shared_ptr<texture> ntex = std::make_shared<texture>();
    ntex->width = src.width;
    ntex->height = src.height;
    ntex->full_width = full_width;
    ntex->full_height = full_height;
    ntex->u = src.x;
    ntex->v = src.y;
    ntex->root = shared_from_this();
    ntex->relying_image_ = relying_image_;
    ntex->texture_id_ = texture_id_;
    ntex->is_framebuffer_ = is_framebuffer_;
    return ntex;
}

void texture::bind_(int unit) {
    if (unit == 0) print_throw(log_level::fatal, "cannot bind to texture unit 0, since it is reserved.");
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}

}  // namespace arc