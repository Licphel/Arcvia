#include "core/buffer.h"
#include "lua/lua.h"
#include "net/packet.h"
#include "net/socket.h"

using namespace arc;

namespace arc {

static std::unordered_map<std::string, lua_function> lua_performers_;

struct lua_packet : packet {
    lua_table data;
    std::string type;

    lua_packet() { data = lua_make_table(); }

    lua_packet(const std::string& type, const lua_table& tb) : type(type), data(tb) {}

    void read(byte_buf& buf) override { data["str"] = buf.read<std::string>(); }
    void write(byte_buf& buf) const override { buf.write<std::string>(data["str"]); }

    void perform(packet_context* ctx) override {
        auto it = lua_performers_.find(type);
        if (it == lua_performers_.end()) print_throw(log_level::fatal, "no performer for {}", type);
        it->second(ctx, data);
    }
};

void lua_bind_net(lua_state& lua) {
    packet::mark_id<lua_packet>();

    auto _n = lua_make_table();

    // component
    auto pkt_type = lua_new_usertype<lua_packet>(_n, "packet_",
                                                 lua_constructors<lua_packet(), lua_packet(std::string, lua_table)>());
    pkt_type["data"] = &lua_packet::data;
    pkt_type["write"] = &lua_packet::write;
    pkt_type["read"] = &lua_packet::read;
    pkt_type["perform"] = &lua_packet::perform;

    // socket
    auto sock_type = lua_new_usertype<socket>(_n, "socket", lua_native);
    sock_type["send_to_server"] = [](socket& self, const std::string& type, const lua_table& tb) {
        self.send_to_server(packet::make<lua_packet>(type, tb));
    };
    sock_type["send_to_remote"] = [](socket& self, const uuid& uuid, const std::string& type, const lua_table& tb) {
        self.send_to_remote(uuid, packet::make<lua_packet>(type, tb));
    };
    sock_type["send_to_remotes"] = lua_combine(
        [](socket& self, const lua_table& uuids, const std::string& type, const lua_table& tb) {
            auto pkt = packet::make<lua_packet>(type, tb);
            for (auto& rid : uuids) self.send_to_remote(rid.second.as<uuid>(), pkt);
        },
        [](socket& self, const std::string& type, const lua_table& tb) {
            self.send_to_remotes(packet::make<lua_packet>(type, tb));
        });
    _n["socket"]["server"] = socket::server;
    _n["socket"]["remote"] = socket::remote;

    _n["add_packet_performer"] = [](const std::string& type, const lua_function& f) { lua_performers_[type] = f; };

    // context
    auto ctx_type = lua_new_usertype<packet_context>(_n, "packet_context", lua_native);
    ctx_type["hold_alive"] = &packet_context::hold_alive;

    lua["arc"]["net"] = _n;
}

}  // namespace arc