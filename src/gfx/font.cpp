#include "gfx/font.h"

#include "core/chcvt.h"
#include "core/io.h"
#include "gfx/atlas.h"
#include "gfx/brush.h"
#include "gfx/image.h"

// clang-format off
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
// clang-format on

namespace arc {

struct font::impl_ {
    FT_FaceRec_* face_ptr;
    FT_LibraryRec_* lib_ptr;
    std::unordered_map<int, std::shared_ptr<atlas>> codemap;
    double res, pix;
    bool filter_smth;
};

font::font() : pimpl_(std::make_unique<impl_>()) {}

font::~font() {
    FT_Done_Face(pimpl_->face_ptr);
    FT_Done_FreeType(pimpl_->lib_ptr);
}

std::shared_ptr<texture> flush_codemap_(font::impl_* p_, char32_t ch, std::shared_ptr<image> img) {
    int code = static_cast<int>(std::floor(static_cast<int>(ch) / 256.0));
    if (p_->codemap.find(code) == p_->codemap.end()) {
        int ats = p_->res * 16;
        p_->codemap[code] = atlas::make(ats, ats);
        p_->codemap[code]->begin();
    }

    auto atl = p_->codemap[code];
    auto tptr = atl->accept(img);
    atl->end();
    return tptr;
}

glyph font::make_glyph(char32_t ch) {
    auto face = pimpl_->face_ptr;
    unsigned int idx = FT_Get_Char_Index(face, ch);

    FT_Set_Pixel_Sizes(face, 0, pimpl_->res);
    FT_Load_Glyph(face, idx, FT_LOAD_DEFAULT);
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    FT_Bitmap_ m0 = face->glyph->bitmap;

    int len = static_cast<int>(m0.width * m0.rows * 4);
    uint8_t* buf = new uint8_t[len];
    for (int i = 0; i < len; i += 4) {
        uint8_t grey = m0.buffer[i / 4];
        buf[i + 0] = 255;
        buf[i + 1] = 255;
        buf[i + 2] = 255;
        buf[i + 3] = grey;
    }

    std::shared_ptr<image> img =
        image::make(static_cast<int>(face->glyph->bitmap.width), static_cast<int>(face->glyph->bitmap.rows), buf);

    double ds = pimpl_->res / pimpl_->pix;

    glyph g;
    g.texpart = flush_codemap_(pimpl_.get(), ch, img);
    texture_parameters pms;
    if (pimpl_->filter_smth)
        pms = texture_parameters(texture_parameter::uv_clamp, texture_parameter::filter_linear,
                                 texture_parameter::filter_linear);
    else
        pms = texture_parameters(texture_parameter::uv_clamp, texture_parameter::filter_nearest,
                                 texture_parameter::filter_nearest);
    g.texpart->parameters(pms);
    g.size.x = ch == ' ' ? face->glyph->metrics.horiAdvance / ds / 64.0 : face->glyph->metrics.width / ds / 64.0;
    g.size.y = face->glyph->metrics.height / ds / 64.0;
    g.advance = face->glyph->advance.x / ds / 64.0;
    g.offset.x = face->glyph->metrics.horiBearingX / ds / 64.0;
#ifdef ARC_Y_IS_DOWN
    g.offset.y = -face->glyph->metrics.horiBearingY / ds / 64.0 + height + face->bbox.yMin / ds / 64.0;
#else
    g.offset.y = face->glyph->metrics.horiBearingY / ds / 64.0 - g.size.y;
#endif

    return g;
}

font_render_bound font::make_vtx(brush* brush, const std::string& u8_str, double x, double y, long align, double max_w,
                                 double scale) {
    static std::u32string cvtbuf_;
    cvt_u32_(u8_str, &cvtbuf_);
    return make_vtx(brush, cvtbuf_, x, y, align, max_w, scale);
}

font_render_bound font::make_vtx(brush* brush, const std::u32string& str, double x, double y, long align, double max_w,
                                 double scale) {
    if (str.length() == 0 || str.length() > INT16_MAX) return {};

    if (align & font_align::left && align & font_align::up) {
        double h_scaled = scale * lspc;
        double w = 0;
        double lw = 0;
        double h = 0;
        double lh = 0;
        double dx = x;
        double dy = y;
        bool endln = false;
        int lns = 1;

        for (int i = 0; i < static_cast<int>(str.length()); i++) {
            char32_t ch = str[i];

            if (ch == '\n' || endln) {
#ifdef ARC_Y_IS_DOWN
                dy += h_scaled;
#else
                dy -= h_scaled;
#endif
                dx = x;
                endln = false;
                w = std::max(w, lw);
                lw = 0;
                h += h_scaled;
                lh = 0;
                lns++;
                continue;
            }

            glyph g = get_glyph(ch) * scale;

            if (dx - x + g.advance >= max_w) {
                endln = true;
                i -= 1;
                i = std::max(0, i - 1);
                continue;
            }

            lh = std::max(g.size.y, lh);
            lw += g.advance;

            if (brush != nullptr)
                brush->draw_texture(g.texpart, {dx + g.offset.x, dy + g.offset.y, g.size.x, g.size.y});
            dx += g.advance;

            if (i == static_cast<int>(str.length()) - 1 || (get_glyph(str[i + 1]) * scale).advance + dx - x >= max_w)
                lw += g.size.x - g.advance;
        }

#ifdef ARC_Y_IS_DOWN
        return font_render_bound(quad::corner(x, y, std::max(lw, w), h + height * scale), lw, lns);
#else
        return font_render_bound(quad::corner(x, dy, std::max(lw, w), h + f_height * scale), lw, lns);
#endif
    }

    // align the positions
    auto qd = make_vtx(nullptr, str, x, y, font_align::normal, max_w, scale);
    double w = qd.region.width, h = qd.region.height;

    if (align & font_align::down) y -= h;
    if (align & font_align::vert_center) y -= h / 2;
    if (align & font_align::right) x -= w;
    if (align & font_align::hori_center) x -= w / 2;

    return make_vtx(brush, str, x, y, font_align::normal, max_w, scale);
}

std::shared_ptr<font> font::load(const path& path_, double h, font_style style) {
    auto fptr = std::make_shared<font>();

    FT_LibraryRec_* lib;
    FT_FaceRec_* face;
    FT_Init_FreeType(&lib);
    FT_New_Face(lib, path_.strp.c_str(), 0, &face);
    FT_Select_Charmap(face, FT_ENCODING_UNICODE);

    fptr->pimpl_->face_ptr = face;
    fptr->pimpl_->lib_ptr = lib;
    fptr->pimpl_->res = style == font_style::regular ? h * ARC_FONT_RES_SCALE : h;
    fptr->pimpl_->pix = h;
    fptr->pimpl_->filter_smth = style == font_style::regular;
    fptr->height = h;
    fptr->ascend = face->ascender / 64.0;
    fptr->descend = face->descender / 64.0;
    fptr->lspc = h + 1;

    return fptr;
}

}  // namespace arc
