#include "gfx/atlas.h"

#include <algorithm>

#include "core/log.h"
#include "core/math.h"
#include "gfx/image.h"

namespace arc {

struct atlas::impl_ {
    std::vector<quad> free_rects;
};

// for unique_ptr<impl_> to refer
atlas::atlas(int w, int h)
    : width(w), height(h), pixels(new uint8_t[width * height * 4]), p_(std::make_unique<impl_>()) {}

// for unique_ptr<impl_> to refer
atlas::~atlas() = default;

void atlas::begin() {
    p_->free_rects.clear();
    p_->free_rects.push_back(quad(0, 0, width, height));

    output_image = image::make(width, height, pixels);
    output_texture = texture::make(nullptr);

    // pre-set
    output_texture->full_width = output_image->width = width;
    output_texture->full_height = output_image->height = height;

    memset(pixels, 0, width * height * 4);
}

void atlas::end() { output_texture->link_data_(output_image); }

std::shared_ptr<texture> atlas::accept(std::shared_ptr<image> image) {
    if (!image || !image->pixels) return nullptr;
    int rw = image->width;
    int rh = image->height;

    std::vector<quad>& free_rects = p_->free_rects;

    int best = -1, best_score = INT_MAX;
    for (size_t i = 0; i < free_rects.size(); ++i) {
        const quad& fr = free_rects[i];
        if (fr.width < rw || fr.height < rh) continue;
        int score = std::min(fr.width - rw, fr.height - rh);
        if (score < best_score) best_score = score, best = static_cast<int>(i);
    }
    if (best == -1) print_throw(log_level::fatal, "atlas is not big enough. please expand it.");

    quad used = free_rects[best];
    int dx = used.x;
    int dy = used.y;

    imgcpy(image, dx, dy);

    quad right = quad(used.x + rw + ARC_ATLAS_PADDING, used.y, used.width - rw, rh);
    quad top = quad(used.x, used.y + rh + ARC_ATLAS_PADDING, used.width, used.height - rh);

    free_rects.erase(free_rects.begin() + best);

    if (right.width > 0 && right.height > 0) free_rects.push_back(right);
    if (top.width > 0 && top.height > 0) free_rects.push_back(top);

    for (size_t i = 0; i < free_rects.size(); ++i) {
        if (free_rects[i].width == 0 || free_rects[i].height == 0) continue;
        for (size_t j = i + 1; j < free_rects.size(); ++j) {
            if (free_rects[j].width == 0 || free_rects[j].height == 0) continue;

            quad& a = free_rects[i];
            quad& b = free_rects[j];

            if (a.x == b.x && a.width == b.width && (a.y + a.height == b.y || b.y + b.height == a.y)) {
                if (a.y < b.y)
                    a.height += b.height;
                else
                    b.height += a.height;
                (a.y < b.y ? b : a).width = 0;
            } else if (a.y == b.y && a.height == b.height && (a.x + a.width == b.x || b.x + b.width == a.x)) {
                if (a.x < b.x)
                    a.width += b.width;
                else
                    b.width += a.width;
                (a.x < b.x ? b : a).height = 0;
            }
        }
    }

    free_rects.erase(std::remove_if(free_rects.begin(), free_rects.end(),
                                    [](const quad& r) { return r.width == 0 || r.height == 0; }),
                     free_rects.end());

    return output_texture->cut(quad(dx, dy, image->width, image->height));
}

void atlas::imgcpy(std::shared_ptr<image> image, int dest_x, int dest_y) {
    // suppose channels are the same. (rgba format)
    for (int y = 0; y < image->height; ++y) {
        for (int x = 0; x < image->width; ++x) {
            int src_index = (y * image->width + x) * 4;
            int dest_index = ((dest_y + y) * this->width + (dest_x + x)) * 4;

            for (int c = 0; c < 4; ++c) pixels[dest_index + c] = image->pixels[src_index + c];
        }
    }
}

std::shared_ptr<atlas> atlas::make(int w, int h) { return std::make_shared<atlas>(w, h); }

}  // namespace arc