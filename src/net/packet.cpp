#include "net/packet.h"

#include "core/buffer.h"
#include "core/io.h"
#include "net/socket.h"

namespace arc {

std::vector<uint8_t> packet::pack(std::shared_ptr<packet> p) {
    // need to de-refer here
    // idk why clang warns me. this way to eliminate the warning.
    auto const& ref = *p;
    auto n = typeid(ref).hash_code();
    auto it = get_packet_map_h2i_().find(n);

    if (it == get_packet_map_h2i_().end()) print_throw(log_level::fatal, "unregistered packet.");

    int pid = it->second;
    byte_buf uncmped_buf;
    uncmped_buf.write<int>(pid);
    p->write(uncmped_buf);

    auto cmped_buf = io::compress(uncmped_buf.to_vector(), compression_level::optimal);
    uncmped_buf.set_write_pos(0);
    uncmped_buf.write<int>(cmped_buf.size());
    uncmped_buf.write_bytes(cmped_buf.data(), cmped_buf.size());
    int size = static_cast<int>(uncmped_buf.size());

    if (size > 32767) print_throw(log_level::fatal, "too large packet with {} bytes!", size);

    return uncmped_buf.to_vector();
}

std::shared_ptr<packet> packet::unpack(byte_buf& buffer, int len) {
    auto dcmped_buf = io::decompress(buffer.read_advance(len));
    byte_buf buf = byte_buf(std::move(dcmped_buf));

    buf.set_write_pos(dcmped_buf.size());
    int pid = buf.read<int>();

    auto it = get_packet_map_i2f_().find(pid);

    if (it == get_packet_map_i2f_().end()) print_throw(log_level::fatal, "unregistered packet.");

    std::shared_ptr<packet> p = it->second();
    p->read(buf);
    return p;
}

void packet::send_to_server() { socket::remote->send_to_server(shared_from_this()); }

void packet::send_to_remote(const uuid& rid) { socket::server->send_to_remote(rid, shared_from_this()); }

void packet::send_to_remotes(const std::vector<uuid>& rids) {
    auto ptr = shared_from_this();
    auto* skt = socket::server;
    for (auto& r : rids) skt->send_to_remote(r, ptr);
}

void packet::send_to_remotes() { socket::server->send_to_remotes(shared_from_this()); }

static int pid_counter_v_;
static std::unordered_map<int, std::function<std::shared_ptr<packet>()>> pmap_v_;
static std::unordered_map<size_t, int> pmap_rev_v_;

int pid_counter_() { return pid_counter_v_++; }

std::unordered_map<int, std::function<std::shared_ptr<packet>()>>& get_packet_map_i2f_() { return pmap_v_; }

std::unordered_map<size_t, int>& get_packet_map_h2i_() { return pmap_rev_v_; }

}  // namespace arc
