#pragma once
#include <unordered_map>

#include "core/io.h"
#include "core/math.h"
#include "gfx/brush.h"
#include "gfx/image.h"

#define ARC_FONT_RES_SCALE 4

namespace arc {

struct font_align {
    constexpr static long left = 1L << 0;
    constexpr static long right = 1L << 1;
    constexpr static long hori_center = 1L << 2;
    constexpr static long up = 1L << 3;
    constexpr static long down = 1L << 4;
    constexpr static long vert_center = 1L << 5;

    constexpr static long normal = left | up;
    constexpr static long normal_center = hori_center | up;
};

struct glyph {
    std::shared_ptr<texture> texpart;
    vec2 size;
    double advance;
    vec2 offset;

    // scale the glyph.
    inline glyph operator*(double scl) {
        return {
            texpart,
            size * scl,
            advance * scl,
            offset * scl,
        };
    }
};

struct font_render_bound {
    quad region;
    double last_width;
    int lines;
};

enum class font_style { regular, pixel };

struct font {
    struct impl_;
    std::unique_ptr<impl_> pimpl_;

    std::unordered_map<char32_t, glyph> glyph_map;
    double height = 0;
    double lspc = 0;
    double ascend = 0;
    double descend = 0;

    font();
    ~font();

    glyph get_glyph(char32_t ch) {
        if (glyph_map.find(ch) == glyph_map.end()) return glyph_map[ch] = make_glyph(ch);
        return glyph_map[ch];
    }

    glyph make_glyph(char32_t ch);
    // draw the stringin the font, return the bounding box.
    // and if brush is nullptr, it won't draw anything, just calculating the
    // bounding box.
    font_render_bound make_vtx(brush* brush, const std::string& str, double x, double y,
                               long align = font_align::normal, double max_w = INT_MAX, double scale = 1);
    font_render_bound make_vtx(brush* brush, const std::u32string& str, double x, double y,
                               long align = font_align::normal, double max_w = INT_MAX, double scale = 1);

    static std::shared_ptr<font> load(const path& path_, double h, font_style style);
};

}  // namespace arc
