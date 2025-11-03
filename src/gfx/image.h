#pragma once
#include <memory>
#include "core/io.h"
#include "core/math.h"

namespace arc {

struct brush;

struct image {
    int width, height;
    uint8_t* pixels;
    /* unstable */ bool is_from_stb_ = false;

    image();
    image(uint8_t* data, int w, int h);
    ~image();

    static std::shared_ptr<image> load(const path& path_);
    static std::shared_ptr<image> make(int width, int height, uint8_t* data);
};

enum class texture_parameter { uv_repeat, uv_clamp, uv_mirror, filter_nearest, filter_linear };

struct texture_parameters {
    texture_parameter uv = texture_parameter::uv_repeat;
    texture_parameter min_filter = texture_parameter::filter_nearest;
    texture_parameter mag_filter = texture_parameter::filter_nearest;
};

struct texture : public std::enable_shared_from_this<texture> {
    int u = 0;
    int v = 0;
    int width = 0;
    int height = 0;
    int full_width = 0;
    int full_height = 0;
    /* maybe nullptr */ std::shared_ptr<image> relying_image_ = nullptr;
    /* unstable */ unsigned int texture_id_ = 0;
    /* unstable */ bool is_framebuffer_;
    std::shared_ptr<texture> root = nullptr;

    ~texture();

    static std::shared_ptr<texture> make(std::shared_ptr<image> img);
    void parameters(texture_parameters param);
    std::shared_ptr<texture> cut(const quad& src);
    void link_data_(std::shared_ptr<image> img);
    void bind_(int i);
};

}  // namespace arc