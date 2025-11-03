#include "core/page.h"

#include "core/buffer.h"

namespace arc {

byte_page::byte_page() = default;
byte_page::byte_page(const byte_buf& buf) : buf_(buf) {}

void byte_page::recover(byte_buf& tmp) {
    tmp.rewind();
    uint32_t dir_ln = tmp.read<uint32_t>();
    pages_.resize(dir_ln);
    tmp.read_bytes(pages_.data(), dir_ln * sizeof(uint32_t));
    uint32_t dat_ln = tmp.read<uint32_t>();
    buf_.clear();
    buf_.resize(dat_ln);
    tmp.read_bytes(buf_.data_.data(), dat_ln);
    buf_.set_write_pos(dat_ln);
}

byte_buf byte_page::snapshot() const {
    byte_buf out;
    uint32_t dir_ln = static_cast<uint32_t>(pages_.size());
    out.write(dir_ln);
    out.write_bytes(pages_.data(), dir_ln * sizeof(uint32_t));
    uint32_t dat_ln = static_cast<uint32_t>(buf_.size());
    out.write(dat_ln);
    out.write_bytes(buf_.to_vector().data(), dat_ln);
    return out;
}

const byte_buf& byte_page::wind() {
    buf_.set_write_pos(buf_.size());
    buf_.set_read_pos(0);
    return buf_;
}

void byte_page::ensure_page(int idx) {
    if (idx < 0) return;
    if (idx >= static_cast<int>(pages_.size())) pages_.resize(idx + 1, 0);
}

}  // namespace arc