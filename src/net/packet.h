#pragma once
#include <functional>
#include <unordered_map>

#include "core/buffer.h"
#include "core/log.h"
#include "core/uuid.h"

#define ARC_USE_BUILTIN_PACKETS

namespace arc {

struct packet_context {
    // signals the remote is still alive.
    // needed to be done every #ARC_NET_TIMEOUT seconds.
    virtual void hold_alive(const uuid& id) = 0;
};

struct packet;

int pid_counter_();
std::unordered_map<int, std::function<std::shared_ptr<packet>()>>& get_packet_map_i2f_();
std::unordered_map<size_t, int>& get_packet_map_h2i_();

struct packet : public std::enable_shared_from_this<packet> {
    uuid sender;

    virtual ~packet() = default;
    virtual void read(byte_buf& buf) = 0;
    virtual void write(byte_buf& buf) const = 0;
    virtual void perform(packet_context* ctx) = 0;

    // packet procotol:
    // unzipped int: LENGTH
    // zipped int: PID
    // zipped int: DATA

    static std::vector<uint8_t> pack(std::shared_ptr<packet> p);
    static std::shared_ptr<packet> unpack(byte_buf& buf, int len);

    void send_to_server();
    void send_to_remote(const uuid& rid);
    void send_to_remotes(const std::vector<uuid>& rids);
    void send_to_remotes();

    template <typename T>
    static void mark_id() {
        const auto& tid = typeid(T);
        int pid = pid_counter_();
        get_packet_map_i2f_()[pid] = []() { return packet::make<T>(); };
        get_packet_map_h2i_()[tid.hash_code()] = pid;
    }

    template <typename T, typename... Args>
    static std::shared_ptr<T> make(Args&&... args) {
        static_assert(std::is_base_of_v<packet, T>, "make a non-packet object.");
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
};

#ifdef ARC_USE_BUILTIN_PACKETS
struct packet_2s_heartbeat : packet {
    packet_2s_heartbeat() = default;

    void read(byte_buf&) override {}
    void write(byte_buf&) const override {}
    void perform(packet_context* ctx) override { ctx->hold_alive(sender); }
};

struct packet_dummy : packet {
    std::string str;

    packet_dummy() = default;
    packet_dummy(const std::string& str) : str(str) {}

    void read(byte_buf& buf) override { str = buf.read<std::string>(); }
    void write(byte_buf& buf) const override { buf.write<std::string>(str); }
    void perform(packet_context*) override { print(log_level::info, "received: " + str); }
};
#endif

}  // namespace arc
