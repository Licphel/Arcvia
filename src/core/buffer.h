#pragma once
#include <cstring>
#include <type_traits>
#include <vector>

#include "core/uuid.h"

namespace arc {

// check if the system is little-endian.
bool check_is_system_little_endian_();

template <typename T>
T swap_endian(T value) {
    if constexpr (sizeof(T) == 1)
        return value;
    else {
        union {
            T value;
            uint8_t bytes[sizeof(T)];
        } src, dst;

        src.value = value;
        for (size_t i = 0; i < sizeof(T); ++i) dst.bytes[i] = src.bytes[sizeof(T) - 1 - i];
        return dst.value;
    }
}

template <typename T>
T to_native_endian(T value) {
    if (check_is_system_little_endian_()) return value;
    return swap_endian(value);
}

template <typename T>
T advance_read_ptr_(uint8_t* ptr) {
    T value;
    std::memcpy(&value, ptr, sizeof(T));
    ptr += sizeof(T);
    return to_native_endian(value);
}

template <typename T>
void advance_write_ptr_(uint8_t* ptr, const T& v) {
    T swapv = to_native_endian(v);
    std::memcpy(ptr, &swapv, sizeof(T));
    ptr += sizeof(T);
}

// general byte buffer used in serialization, networking, etc.
// endian-aware.
struct byte_buf {
    std::vector<uint8_t> data_;
    size_t rpos_ = 0;
    size_t wpos_ = 0;

    byte_buf();
    byte_buf(size_t initial_size);
    byte_buf(const std::vector<uint8_t>& vec);
    byte_buf(const byte_buf& cpy);

    size_t size() const;
    size_t capacity() const;
    size_t free_bytes() const;
    size_t remaining() const;
    size_t readable_bytes() const;
    bool is_empty() const;
    void clear();
    void reserve(size_t size);
    void resize(size_t size);
    void ensure_capacity(size_t needed);
    void ensure_readable(size_t needed) const;

    template <typename T>
    void write(const T& v) {
        using U = std::decay_t<T>;
        if constexpr (std::is_arithmetic_v<U>) {
            ensure_capacity(sizeof(T));
            T swapv = to_native_endian(v);
            std::memcpy(data_.data() + wpos_, &swapv, sizeof(U));
            wpos_ += sizeof(U);
        } else if constexpr (std::is_constructible_v<std::string, U>) {
            write_string(std::string(v));
        } else if constexpr (std::is_same_v<U, std::string>) {
            write_string(v);
        } else if constexpr (std::is_same_v<U, byte_buf>) {
            write_byte_buf(v);
        } else if constexpr (std::is_same_v<U, uuid>) {
            write_uuid(v);
        } else {
            static_assert(false, "unsupported type");
        }
    }

    template <typename T>
    T read() {
        if constexpr (std::is_arithmetic_v<T>) {
            ensure_readable(sizeof(T));
            T value;
            std::memcpy(&value, data_.data() + rpos_, sizeof(T));
            rpos_ += sizeof(T);
            return to_native_endian(value);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return static_cast<std::string>(read_string());
        } else if constexpr (std::is_same_v<T, byte_buf>) {
            return static_cast<byte_buf>(read_byte_buf());
        } else if constexpr (std::is_same_v<T, uuid>) {
            return static_cast<uuid>(read_uuid());
        } else {
            static_assert(false, "unsupported type");
        }
    }

    void write_bytes(const void* src, size_t len);
    void read_bytes(void* dst, size_t len);

   private:
    void write_byte_buf(const byte_buf& buf);
    void write_string(const std::string& str);
    void write_uuid(const uuid& id);

    byte_buf read_byte_buf();
    std::string read_string();
    uuid read_uuid();

   public:
    void skip(size_t len);
    void set_read_pos(size_t pos);
    void set_write_pos(size_t pos);
    size_t read_pos() const;
    size_t write_pos() const;
    // copy the data before write position to a vector.
    std::vector<uint8_t> to_vector() const;
    // read from current read position and advance it by len.
    std::vector<uint8_t> read_advance(int len);
    // rewind so that we can read the data from the very beginning.
    void rewind();
    // remove the read data and move the unread data to the front.
    void compact();
};

}  // namespace arc
