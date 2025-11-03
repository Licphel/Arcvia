#include "core/buffer.h"

#include "core/log.h"
#include "core/uuid.h"

namespace arc {

constexpr uint32_t letest_ = 0x01020304;
static bool le_ = reinterpret_cast<const uint8_t*>(&letest_)[0] == 0x04;

bool check_is_system_little_endian_() { return le_; }

byte_buf::byte_buf() = default;

byte_buf::byte_buf(size_t initial_size) : data_(initial_size) {}

byte_buf::byte_buf(const std::vector<uint8_t>& vec) {
    data_ = vec;
    wpos_ = vec.size();
}

byte_buf::byte_buf(const byte_buf& cpy) : byte_buf(cpy.to_vector()) {}
size_t byte_buf::size() const { return wpos_; }
size_t byte_buf::capacity() const { return data_.capacity(); }
size_t byte_buf::free_bytes() const { return data_.capacity() - wpos_; }
size_t byte_buf::remaining() const { return data_.size() - wpos_; }
size_t byte_buf::readable_bytes() const { return wpos_ - rpos_; }
bool byte_buf::is_empty() const { return wpos_ == 0; }

void byte_buf::clear() {
    rpos_ = 0;
    wpos_ = 0;
}

void byte_buf::reserve(size_t size) { data_.reserve(size); }

void byte_buf::resize(size_t size) {
    data_.resize(size);
    if (wpos_ > size) wpos_ = size;
    if (rpos_ > size) rpos_ = size;
}

void byte_buf::ensure_capacity(size_t needed) {
    if (remaining() < needed) data_.resize(data_.size() + std::max(needed, data_.size() * 2));
}

void byte_buf::ensure_readable(size_t needed) const {
    if (readable_bytes() < needed) print_throw(log_level::fatal, "byte buffer read out of range!");
}

void byte_buf::write_bytes(const void* src, size_t len) {
    ensure_capacity(len);
    std::memcpy(data_.data() + wpos_, src, len);
    wpos_ += len;
}

void byte_buf::write_byte_buf(const byte_buf& buf) {
    write<unsigned int>((unsigned int)buf.size());
    write_bytes(buf.data_.data(), buf.size());
}

void byte_buf::write_string(const std::string& str) {
    write<unsigned int>(str.size());
    write_bytes(str.data(), str.size());
}

void byte_buf::write_uuid(const uuid& id) { write_bytes(id.bytes, 16); }

void byte_buf::read_bytes(void* dst, size_t len) {
    ensure_readable(len);
    std::memcpy(dst, data_.data() + rpos_, len);
    rpos_ += len;
}

byte_buf byte_buf::read_byte_buf() {
    ensure_readable(sizeof(unsigned int));
    size_t size = read<unsigned int>();
    byte_buf buf = byte_buf(size);
    read_bytes(buf.data_.data(), size);
    buf.set_write_pos(size);
    return buf;
}

std::string byte_buf::read_string() {
    ensure_readable(sizeof(unsigned int));
    size_t len = read<unsigned int>();
    ensure_readable(len);
    std::string str(reinterpret_cast<const char*>(data_.data() + rpos_), len);
    rpos_ += len;
    return str;
}

uuid byte_buf::read_uuid() {
    ensure_readable(16);
    uuid u;
    read_bytes(u.bytes, 16);
    return u;
}

void byte_buf::skip(size_t len) {
    ensure_readable(len);
    rpos_ += len;
}

void byte_buf::set_read_pos(size_t pos) {
    if (pos > wpos_) print_throw(log_level::fatal, "byte buf read pos out of range!");
    rpos_ = pos;
}

void byte_buf::set_write_pos(size_t pos) {
    if (pos > data_.size()) data_.resize(pos);
    wpos_ = pos;
    if (rpos_ > wpos_) rpos_ = wpos_;
}

size_t byte_buf::read_pos() const { return rpos_; }
size_t byte_buf::write_pos() const { return wpos_; }

std::vector<uint8_t> byte_buf::to_vector() const {
    if (wpos_ > data_.size()) print_throw(log_level::fatal, "byte buf write pos out of range!");
    return std::vector<uint8_t>(data_.begin(), data_.begin() + wpos_);
}

std::vector<uint8_t> byte_buf::read_advance(int len) {
    ensure_readable(len);
    auto vec = std::vector<uint8_t>(data_.begin() + rpos_, data_.begin() + rpos_ + len);
    rpos_ += len;
    return vec;
}

void byte_buf::rewind() { rpos_ = 0; }

void byte_buf::compact() {
    if (rpos_ > 0) {
        size_t remaining = wpos_ - rpos_;
        if (remaining > 0) std::memmove(data_.data(), data_.data() + rpos_, remaining);
        wpos_ = remaining;
        rpos_ = 0;
    }
}

}  // namespace arc
