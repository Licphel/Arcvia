#include "core/chcvt.h"

#include "core/log.h"

namespace arc {

void cvt_u32_(const std::string& u8_str, std::u32string* u32_tmp) {
    u32_tmp->clear();
    u32_tmp->reserve(u8_str.size());
    auto it = u8_str.begin(), last = u8_str.end();

    auto need = [&](std::ptrdiff_t n) { return std::distance(it, last) >= n; };
    auto cont = [&](unsigned char c) { return (c & 0xC0) == 0x80; };
    auto log = [&]() { print(log_level::warn, "u8 -> u32 charset converting failed."); };

    while (it != last) {
        unsigned char lead = *it++;
        char32_t cp;
        if (lead < 0x80)
            cp = lead;
        else if ((lead & 0xE0) == 0xC0) {
            if (!need(1) || !cont(*it)) {
                log();
                return;
            }
            cp = char32_t(lead & 0x1F) << 6 | (*it++ & 0x3F);
            if (cp < 0x80) {
                log();
                return;
            }
        } else if ((lead & 0xF0) == 0xE0) {
            if (!need(2) || !cont(it[0]) || !cont(it[1])) {
                log();
                return;
            }
            cp = char32_t(lead & 0x0F) << 12 | char32_t(*it++ & 0x3F) << 6 | char32_t(*it++ & 0x3F);
            if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) {
                log();
                return;
            }
        } else if ((lead & 0xF8) == 0xF0) {
            if (!need(3) || !cont(it[0]) || !cont(it[1]) || !cont(it[2])) {
                log();
                return;
            }
            cp = char32_t(lead & 0x07) << 18 | char32_t(*it++ & 0x3F) << 12 | char32_t(*it++ & 0x3F) << 6 |
                 char32_t(*it++ & 0x3F);
            if (cp < 0x10000 || cp > 0x10FFFF) {
                log();
                return;
            }
        } else {
            log();
            return;
        }
        u32_tmp->push_back(cp);
    }
}

void cvt_u8_(const std::u32string& u32_str, std::string* u8_tmp) {
    u8_tmp->clear();
    u8_tmp->reserve(u32_str.size() * 4);

    auto push = [u8_tmp](unsigned char byte) { u8_tmp->push_back(static_cast<char>(byte)); };
    auto log = [&]() { print(log_level::warn, "u32 -> u8 charset converting failed."); };

    for (char32_t cp : u32_str) {
        if (cp <= 0x7F) {
            push(static_cast<unsigned char>(cp));
        } else if (cp <= 0x7FF) {
            push(0xC0 | (cp >> 6));
            push(0x80 | (cp & 0x3F));
        } else if (cp <= 0xFFFF) {
            push(0xE0 | (cp >> 12));
            push(0x80 | ((cp >> 6) & 0x3F));
            push(0x80 | (cp & 0x3F));
        } else if (cp <= 0x10FFFF) {
            push(0xF0 | (cp >> 18));
            push(0x80 | ((cp >> 12) & 0x3F));
            push(0x80 | ((cp >> 6) & 0x3F));
            push(0x80 | (cp & 0x3F));
        } else {
            log();
            return;
        }
    }
}

}  // namespace arc
