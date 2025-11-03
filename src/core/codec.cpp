#include "core/codec.h"

#include <core/buffer.h>
#include <core/codec.h>
#include <core/io.h>

#include <cstdint>
#include <string>

namespace arc {

void write_map_(byte_buf& buf, const codec_map& map);
void write_array_(byte_buf& buf, const codec_vector& arr);

void write_primitive_(byte_buf& buf, const variant_& v) {
    buf.write<uint8_t>((uint8_t)v.type);

    switch (v.type) {
        case codec_type_::b:
            buf.write<bool>(v.cast<bool>());
            break;
        case codec_type_::n:
            buf.write<double>(v.cast<double>());
            break;
        case codec_type_::s:
            buf.write<std::string>(v.cast<std::string>());
            break;
        case codec_type_::m:
            write_map_(buf, v.cast<codec_map>());
            break;
        case codec_type_::v:
            write_array_(buf, v.cast<codec_vector>());
            break;
        case codec_type_::f:
            buf.write<byte_buf>(v.cast<byte_buf>());
            break;
        case codec_type_::u:
            buf.write<uuid>(v.cast<uuid>());
            break;
        default:
            break;
    }
}

void write_map_(byte_buf& buf, const codec_map& map) {
    for (auto& kv : map.data) {
        std::string str = kv.first;
        variant_ v = kv.second;

        write_primitive_(buf, v);
        buf.write<std::string>(str);
    }

    buf.write<uint8_t>(255);
}

void write_array_(byte_buf& buf, const codec_vector& arr) {
    buf.write<size_t>(arr.size());
    for (auto& bv : arr.data) write_primitive_(buf, bv);
}

codec_map read_map_(byte_buf& buf);
codec_vector read_array_(byte_buf& buf);

variant_ read_primitive_(uint8_t id, byte_buf& buf) {
    switch ((codec_type_)id) {
        case codec_type_::b:
            return variant_::make(buf.read<bool>());
        case codec_type_::n:
            return variant_::make(buf.read<double>());
        case codec_type_::s:
            return variant_::make(buf.read<std::string>());
        case codec_type_::m:
            return variant_::make(read_map_(buf));
        case codec_type_::v:
            return variant_::make(read_array_(buf));
        case codec_type_::f:
            return variant_::make(buf.read<byte_buf>());
        case codec_type_::u:
            return variant_::make(buf.read<uuid>());
        default:
            print_throw(log_level::fatal, "unknown binary id.");
    }
}

codec_map read_map_(byte_buf& buf) {
    codec_map map;
    while (true) {
        uint8_t id = buf.read<uint8_t>();
        if (id == 255) break;
        variant_ bv = read_primitive_(id, buf);

        std::string str = buf.read<std::string>();
        map.data[str] = bv;
    }
    return map;
}

codec_vector read_array_(byte_buf& buf) {
    size_t size = buf.read<size_t>();
    codec_vector arr;
    while (size-- > 0) arr.data.push_back(read_primitive_(buf.read<uint8_t>(), buf));
    return arr;
}

codec_map codec_map::load(byte_buf& v) { return read_map_(v); }

byte_buf codec_map::write() const {
    byte_buf buf;
    write_map_(buf, *this);
    return buf;
}

codec_map codec_map::load(const path& path_) {
    byte_buf buf = byte_buf(io::read_bytes(path_, compression_level::dcmp_read));
    return load(buf);
}

void codec_map::write(const path& path_) const {
    io::write_bytes(path_, write().to_vector(), compression_level::optimal);
}

class binparser_ {
   private:
    std::string input;
    size_t pos;

    void skipspace_() {
        while (pos < input.size() && std::isspace(input[pos])) pos++;
    }

    char cur_ch_() { return (pos < input.size()) ? input[pos] : '\0'; }

    char nxt_() {
        pos++;
        return cur_ch_();
    }

    variant_ parse_value_();
    variant_ parse_bool_();
    variant_ parse_num_();
    std::string parse_key_();
    variant_ parse_str_();
    variant_ parse_arr_();
    variant_ parse_map_();

   public:
    binparser_(const std::string& str) : input(str), pos(0) {
        skipspace_();
        while (cur_ch_() != '{') nxt_();
    }

    variant_ parse_term_() {
        skipspace_();
        return parse_value_();
    }
};

variant_ binparser_::parse_value_() {
    skipspace_();
    char c = cur_ch_();

    if (c == 'n') print_throw(log_level::fatal, "cannot use a null value");
    if (c == 't' || c == 'f') return parse_bool_();
    if (c == '"') return parse_str_();
    if (c == '[') return parse_arr_();
    if (c == '{') return parse_map_();
    if (c == '-' || (c >= '0' && c <= '9')) return parse_num_();

    print_throw(log_level::fatal, "unexpected character at position {}", pos);
}

variant_ binparser_::parse_bool_() {
    if (input.substr(pos, 4) == "true") {
        pos += 4;
        return variant_::make(true);
    }
    if (input.substr(pos, 5) == "false") {
        pos += 5;
        return variant_::make(false);
    }
    print_throw(log_level::fatal, "expected boolean at position {}", pos);
}

variant_ binparser_::parse_num_() {
    size_t start = pos;

    if (cur_ch_() == '-') nxt_();

    while (pos < input.size() && std::isdigit(input[pos])) nxt_();

    if (cur_ch_() == '.') {
        nxt_();
        while (pos < input.size() && std::isdigit(input[pos])) nxt_();
    }

    if (cur_ch_() == 'e' || cur_ch_() == 'E') {
        nxt_();
        if (cur_ch_() == '+' || cur_ch_() == '-') nxt_();
        while (pos < input.size() && std::isdigit(input[pos])) nxt_();
    }

    std::string numStr = input.substr(start, pos - start);
    double value = std::stod(numStr);
    return variant_::make(value);
}

std::string binparser_::parse_key_() {
    std::string result;
    while (pos < input.size() && cur_ch_() != '=') {
        char ch = cur_ch_();
        if (!std::isspace(ch)) result += ch;
        nxt_();
    }
    return result;
}

variant_ binparser_::parse_str_() {
    if (cur_ch_() != '"') print_throw(log_level::fatal, "expected '\"' at position {}", pos);
    nxt_();

    std::string result;
    while (pos < input.size() && cur_ch_() != '"') {
        char c = cur_ch_();

        if (c == '\\') {
            nxt_();
            c = cur_ch_();
            switch (c) {
                case '"':
                    result += '"';
                    break;
                case '\\':
                    result += '\\';
                    break;
                case '/':
                    result += '/';
                    break;
                case 'b':
                    result += '\b';
                    break;
                case 'f':
                    result += '\f';
                    break;
                case 'n':
                    result += '\n';
                    break;
                case 'r':
                    result += '\r';
                    break;
                case 't':
                    result += '\t';
                    break;
                case 'u':
                    nxt_();
                    for (int i = 0; i < 4 && pos < input.size(); i++) {
                        nxt_();
                    }
                    result += '?';
                    break;
                default:
                    result += c;
                    break;
            }
        } else
            result += c;
        nxt_();
    }

    if (cur_ch_() != '"') print_throw(log_level::fatal, "unterminated std::stringat position {}", pos);
    nxt_();

    return variant_::make(result);
}

variant_ binparser_::parse_arr_() {
    if (cur_ch_() != '[') print_throw(log_level::fatal, "expected '[' at position {}", pos);
    nxt_();

    codec_vector result;
    skipspace_();

    if (cur_ch_() == ']') {
        nxt_();
        return variant_::make(result);
    }

    while (pos < input.size()) {
        result.data.push_back(parse_value_());

        skipspace_();
        if (cur_ch_() == ']') break;
        if (cur_ch_() != ',') print_throw(log_level::fatal, "expected ',' or ']' in array at position {}", pos);
        nxt_();
        skipspace_();
    }

    if (cur_ch_() != ']') print_throw(log_level::fatal, "unterminated array at position {}", pos);
    nxt_();

    return variant_::make(result);
}

variant_ binparser_::parse_map_() {
    if (cur_ch_() != '{') print_throw(log_level::fatal, "expected '{' at position {}", pos);
    nxt_();

    codec_map result;
    skipspace_();

    if (cur_ch_() == '}') {
        nxt_();
        return variant_::make(result);
    }

    while (pos < input.size()) {
        skipspace_();
        std::string key = parse_key_();
        skipspace_();
        if (cur_ch_() != '=') print_throw(log_level::fatal, "expected '=' after object key at position {}", pos);
        nxt_();

        variant_ value = parse_value_();
        result.data[key] = value;

        skipspace_();

        if (cur_ch_() == '}') break;
        if (cur_ch_() != ',') print_throw(log_level::fatal, "expected ',' or '}' in object at position {}", pos);

        nxt_();
    }

    if (cur_ch_() != '}') print_throw(log_level::fatal, "unterminated object at position {}", pos);
    nxt_();

    return variant_::make(result);
}

codec_map codec_map::load_lang(const path& path_) {
    variant_ result = binparser_(io::read_str(path_)).parse_term_();
    if (result.type != codec_type_::m) print_throw(log_level::fatal, "binary root is not an object");
    return result.cast<codec_map>();
}

}  // namespace arc
