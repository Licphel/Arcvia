#pragma once
#include <cstdint>
#include <string>
#include <type_traits>
#include <vector>

#include "core/buffer.h"

namespace arc {

// byte page is a byte blob that can be indexed.
// it's a bit like a codec, but the key is uint.
struct byte_page {
    std::vector<uint32_t> pages_;
    byte_buf buf_;

    byte_page();
    byte_page(const byte_buf& buf);

    template <typename T>
    uint32_t tell_size(const T& v) {
        using U = std::decay_t<T>;
        if constexpr (std::is_arithmetic_v<U>) {
            return sizeof(v);
        } else if constexpr (std::is_constructible_v<std::string, U>) {
            return std::string(v).size() + 4;  // a 4-byte 'size'
        } else if constexpr (std::is_same_v<U, std::string>) {
            return v.size() + sizeof(uint32_t);  // a 4-byte 'size'
        } else if constexpr (std::is_same_v<U, byte_buf>) {
            return v.size() + sizeof(uint32_t);  // a 4-byte 'size'
        } else if constexpr (std::is_same_v<U, byte_page>) {
            return sizeof(uint32_t) * 2 + v.buf_.size() + +v.pages_.size() * sizeof(uint32_t);
        } else if constexpr (std::is_same_v<U, uuid>) {
            return sizeof(v.bytes);
        } else {
            static_assert(false, "unsupported type");
        }
    }

    template <typename T>
    void write(int idx, const T& value) {
        ensure_page(idx);
        uint32_t old_off = pages_[idx];
        uint32_t need = tell_size(value);
        int opage_size = static_cast<int>(pages_.size());
        uint32_t new_off = (idx + 1 < opage_size) ? pages_[idx + 1] : buf_.size();

        // move bytes
        if (new_off - old_off < need) {
            uint32_t shift = need - (new_off - old_off);
            buf_.set_write_pos(new_off);
            buf_.ensure_capacity(shift);
            std::memmove(buf_.data_.data() + new_off + shift, buf_.data_.data() + new_off, buf_.size() - new_off);
            for (int i = idx + 1; i < opage_size; ++i) pages_[i] += shift;
            new_off += shift;
        }

        buf_.set_write_pos(old_off);
        if constexpr (std::is_same_v<T, byte_page>) {
            buf_.write<byte_buf>(value.snapshot());
        } else {
            buf_.write<T>(value);
        }

        if (idx + 1 == opage_size)
            pages_.push_back(old_off + need);
        else
            pages_[idx + 1] = old_off + need;
    }

    template <typename T>
    T read(int idx, const T& def = T{}) {
        int opage_size = static_cast<int>(pages_.size());
        if (idx < 0 || idx >= opage_size) return def;

        uint32_t dx = pages_[idx];
        uint32_t next = (idx + 1 < opage_size) ? pages_[idx + 1] : buf_.size();
        buf_.set_read_pos(dx);

        if constexpr (std::is_same_v<T, byte_page>) {
            return byte_page(buf_.read<byte_buf>());
        } else {
            return buf_.read<T>();
        }
    }

    template <typename T>
    T read_at(uint32_t dx) {
        buf_.set_read_pos(dx);
        return buf_.read<T>();
    }

    void recover(byte_buf& tmp);
    byte_buf snapshot() const;
    const byte_buf& wind();
    void ensure_page(int idx);
};

}  // namespace arc