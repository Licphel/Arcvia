#include "net/socket.h"

#include <fmt/format.h>

#include <cstddef>
#include <memory>
#include <queue>

#include "core/buffer.h"
#include "core/log.h"
#include "core/time.h"

// boost.asio include
#include <boost/asio.hpp>

namespace arc {

static size_t ARC_NET_BUF_SIZE = 1024 * 1024;
static double ARC_NET_TIMEOUT = 5.0;

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using udp = asio::ip::udp;

template <typename T>
class blocking_queue_ {
   private:
    size_t capacity;
    std::queue<T> que;
    mutable std::mutex mtx;
    std::condition_variable cv_empty;
    std::condition_variable cv_full;

   public:
    explicit blocking_queue_(size_t cap = 0) : capacity(cap) {}

    void push(T item) {
        std::unique_lock<std::mutex> lk(mtx);
        if (capacity > 0) cv_full.wait(lk, [this] { return que.size() < capacity; });
        que.push(std::move(item));
        cv_empty.notify_one();
    }

    T take() {
        std::unique_lock<std::mutex> lk(mtx);
        cv_empty.wait(lk, [this] { return !que.empty(); });
        T val = std::move(que.front());
        que.pop();
        cv_full.notify_one();
        return val;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lk(mtx);
        return que.size();
    }
};

struct socket::impl_ {
    asio::io_context ioc;
    std::thread worker;
    std::thread ioc_worker;
    std::mutex mtx;

    tcp::socket client_sock{ioc};
    byte_buf rcvbuf = byte_buf(ARC_NET_BUF_SIZE);

    tcp::acceptor acceptor{ioc};
    udp::socket broadcaster{ioc};
    std::thread broadcast_thread;
    uint16_t broadcast_port = ARC_UDP_BC_PORT_;

    struct channel : public std::enable_shared_from_this<channel> {
        tcp::socket sock;
        byte_buf rcvbuf = byte_buf(ARC_NET_BUF_SIZE);
        blocking_queue_<std::shared_ptr<packet>> snd_packets;
        uuid id;
        std::thread worker;
        bool is_term = false;
        double last_beat;

        channel() = delete;
        channel(asio::io_context& ioc, uuid uid) : sock(ioc), id(uid) {}

        void wake_() {
            worker = std::thread([self = shared_from_this()] { self->write_(); });
        }

        void write_() {
            auto pkt = snd_packets.take();
            if (pkt == nullptr) return;
            auto buf = packet::pack(pkt);
            auto self = weak_from_this();
            asio::async_write(sock, asio::buffer(buf), [self](std::error_code ec, size_t) {
                if (auto shared_self = self.lock()) {
                    if (ec) print(log_level::warn, "fail to write async: {}", ec.message());
                    if (shared_self->is_term) return;
                    asio::post(shared_self->sock.get_executor(), [self] {
                        if (auto shared_self = self.lock()) shared_self->write_();
                    });
                }
            });
        }
    };

    std::unordered_map<uuid, std::shared_ptr<channel>> channels;
    bool is_server = false;
    bool is_remote = false;
    std::atomic_bool is_term = false;
    std::vector<std::shared_ptr<packet>> rcv_packets;
    blocking_queue_<std::shared_ptr<packet>> snd_packets;
    double last_sec_event;

    impl_() : ioc(), client_sock(ioc), acceptor(ioc), broadcaster(ioc) {}

    ~impl_() {
        if (is_term) return;
        is_term = true;

        acceptor.cancel();
        broadcaster.cancel();
        broadcaster.close();
        client_sock.cancel();
        client_sock.close();

        for (auto& [id, ch] : channels) {
            ch->sock.cancel();
            ch->sock.close();
            ch->is_term = true;
            ch->snd_packets.push(nullptr);
        }

        ioc.stop();
        if (!ioc.stopped()) asio::post(ioc, [] {});

        if (ioc_worker.joinable()) ioc_worker.join();
        if (worker.joinable()) worker.join();
        if (broadcast_thread.joinable()) broadcast_thread.join();

        for (auto& [id, ch] : channels)
            if (ch->worker.joinable()) ch->worker.join();
    }

    void remote_connect(const std::string& host, uint16_t port) {
        is_remote = true;
        tcp::resolver resolver(ioc);
        auto endpoints = resolver.resolve(host, std::to_string(port));

        asio::async_connect(client_sock, endpoints, [this, host, port](std::error_code ec, tcp::endpoint) {
            if (!ec) {
                print(log_level::info, "[remote] successfully connected to {}:{}", host, port);
                remote_read();
            } else
                print_throw(log_level::warn, "[remote] cannot connect to {}:{}, cause: {}", host, port, ec.message());
        });

        worker = std::thread([this] {
            while (!is_term) {
                auto pkt = snd_packets.take();
                if (pkt == nullptr) break;
                auto buf = packet::pack(pkt);
                asio::async_write(client_sock, asio::buffer(buf), [](std::error_code ec, size_t) {
                    if (ec) print(log_level::warn, "fail to write async: {}", ec.message());
                });
            }
        });
        ioc_worker = std::thread([this] { ioc.run(); });
    }

    void remote_disconnect() {
        if (is_remote == false || is_term) return;

        is_remote = false;
        is_term = true;

        client_sock.cancel();
        client_sock.close();

        snd_packets.push(nullptr);
        if (worker.joinable()) worker.join();

        ioc.stop();
        if (ioc_worker.joinable()) ioc_worker.join();

        print(log_level::info, "[remote] remote disconnected.");
    }

    void read_to_queue_(int byte_read, byte_buf& buf, const uuid& id) {
        if (byte_read == 0) {
            if (is_remote) remote_disconnect();
            if (is_server) server_stop();
            return;
        }

        buf.set_write_pos(buf.write_pos() + byte_read);

        while (buf.readable_bytes() >= 4) {
            int rp = buf.read_pos();
            int len = buf.read<int>();

            if (buf.readable_bytes() < (size_t)len) {
                buf.set_read_pos(rp);

                if (buf.capacity() - buf.size() <= (size_t)len + 4) buf.compact();
                break;
            }

            std::shared_ptr<packet> p = packet::unpack(buf, len);
            p->sender = id;

            if (buf.readable_bytes() <= 0)
                buf.clear();
            else if (buf.read_pos() >= buf.capacity() / 2)
                buf.compact();

            // lock is necessary.
            std::lock_guard<std::mutex> lk(mtx);
            rcv_packets.push_back(p);
        }
    }

    void remote_send(std::shared_ptr<packet> pkt) { snd_packets.push(pkt); }

    void remote_read() {
        client_sock.async_read_some(asio::buffer(rcvbuf.data_.data() + rcvbuf.write_pos(), rcvbuf.free_bytes()),
                                    [this](std::error_code ec, size_t n) {
                                        if (ec) {
                                            print(log_level::warn, "[remote] connection is denied.");
                                            return;
                                        }
                                        read_to_queue_(n, rcvbuf, uuid::empty());
                                        remote_read();
                                        if (is_term) return;
                                    });
    }

    void start_broadcast(uint16_t game_port) {
        broadcast_port = ARC_UDP_BC_PORT_;
        broadcaster.open(udp::v4());
        broadcaster.set_option(udp::socket::broadcast(true));
        broadcast_thread = std::thread([this, game_port]() {
            udp::endpoint broadcast_ep(asio::ip::make_address("255.255.255.255"), broadcast_port);
            while (!is_term) {
                std::string msg = "arcvia server " + std::to_string(game_port);
                broadcaster.send_to(asio::buffer(msg), broadcast_ep);
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });
    }

    void stop_broadcast() {
        if (broadcast_thread.joinable()) {
            broadcaster.close();
            broadcast_thread.join();
        }
    }

    void server_start(uint16_t port) {
        print(log_level::info, "[server] server is opened at port {}", port);

        is_server = true;

        tcp::endpoint ep(tcp::v4(), port);
        acceptor.open(ep.protocol());
        acceptor.set_option(tcp::acceptor::reuse_address(true));
        acceptor.bind(ep);
        acceptor.listen();
        server_accept();

        start_broadcast(port);

        ioc_worker = std::thread([this] { ioc.run(); });
    }

    void server_stop() {
        if (!is_server || is_term) return;

        is_server = false;
        is_term = true;

        acceptor.cancel();
        acceptor.close();
        stop_broadcast();

        for (auto& [id, ch] : channels) {
            ch->sock.cancel();
            ch->sock.close();
            ch->is_term = true;
            ch->snd_packets.push(nullptr);
        }

        ioc.stop();

        if (ioc_worker.joinable()) ioc_worker.join();
        if (worker.joinable()) worker.join();

        for (auto& [id, ch] : channels)
            if (ch->worker.joinable()) ch->worker.join();

        channels.clear();
        print(log_level::info, "[server] server is stopped.");
    }

    void server_accept() {
        auto remote = std::make_shared<channel>(ioc, uuid::make());
        remote->wake_();

        acceptor.async_accept(remote->sock, [this, remote](std::error_code ec) {
            if (!ec) {
                print(log_level::info, "[server] server has connected remote {}", (std::string)remote.get()->id);
                server_read(remote);
                channels[remote->id] = remote;
            }
            if (is_term) return;
            server_accept();
        });
    }

    void server_read(std::shared_ptr<channel> r) {
        r->sock.async_read_some(asio::buffer(r->rcvbuf.data_.data() + r->rcvbuf.write_pos(), r->rcvbuf.free_bytes()),
                                [this, r](std::error_code ec, size_t n) {
                                    if (ec) {
                                        channels.erase(r->id);
                                        r->is_term = true;
                                        print(log_level::info, "[server] connection lost: {}", (std::string)r->id);
                                        return;
                                    }
                                    if (is_term) return;
                                    read_to_queue_(n, r->rcvbuf, r->id);
                                    server_read(r);
                                });
    }

    void server_send(const uuid& rid, std::shared_ptr<packet> pkt) {
        auto it = channels.find(rid);
        if (it == channels.end()) return;
        it->second->snd_packets.push(pkt);
    }

    void server_send_every(std::shared_ptr<packet> pkt) {
        for (auto& [id, r] : channels) r->snd_packets.push(pkt);
    }

    void tick(socket* sk) {
        double now = clock::now().seconds;

        if (now - last_sec_event > 1.0) {
            last_sec_event = now;

            if (is_server) {
                auto it = channels.begin();
                while (it != channels.end()) {
                    auto& c = *it;
                    if (now - c.second->last_beat > ARC_NET_TIMEOUT) {
                        print(log_level::info, "remote channel {} timeout.", (std::string)c.second->id);
                        it = channels.erase(it);
                    } else
                        ++it;
                }
            }
            if (is_remote) remote_send(packet::make<packet_2s_heartbeat>());
        }

        // lock is necessary.
        std::vector<std::shared_ptr<packet>> tmp;

        std::lock_guard<std::mutex> lk(mtx);
        tmp.swap(rcv_packets);

        for (auto& e : tmp) e->perform(sk);
    }

    void hold_alive(const uuid& rid) {
        auto it = channels.find(rid);
        if (it == channels.end()) return;
        it->second->last_beat = clock::now().seconds;
    }
};

socket::socket() : pimpl_(std::make_unique<impl_>()) {}

socket::~socket() { stop(); }

void socket::discover() {
    udp::socket udp_sock(pimpl_->ioc);
    udp_sock.open(udp::v4());
    udp_sock.bind(udp::endpoint(asio::ip::address_v4::any(), ARC_UDP_BC_PORT_));

    std::string addrs;
    uint16_t ports = 0;

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);

    while (std::chrono::steady_clock::now() < deadline) {
        std::array<char, 256> recv_buf;
        udp::endpoint sender_ep;
        std::error_code ec;
        std::size_t len = udp_sock.receive_from(asio::buffer(recv_buf), sender_ep);

        if (!ec && len > 0) {
            std::string msg(recv_buf.data(), len);
            const std::string prefix = "arcvia server ";
            if (msg.rfind(prefix, 0) == 0) {
                addrs = sender_ep.address().to_string();
                ports = static_cast<uint16_t>(std::stoi(msg.substr(prefix.size())));
                break;
            }
        }
    }
    udp_sock.close();

    if (!addrs.empty() && ports != 0) {
        print(log_level::info, "[remote] found lan server {}:{}, trying to connect now.", addrs, ports);
        pimpl_->remote_connect(addrs, ports);
    } else
        print(log_level::warn, "[remote] cannot find lan server nearby.");
}

void socket::connect(connection_type type, const std::string& host, uint16_t port) {
    if (type == connection_type::integrated)
        pimpl_->remote_connect("127.0.0.1", gen_tcp_port_());
    else if (type == connection_type::lan)
        discover();
    else if (type == connection_type::addrf)
        pimpl_->remote_connect(host, port);
}

void socket::send_to_server(std::shared_ptr<packet> pkt) { pimpl_->remote_send(pkt); }
void socket::disconnect() { pimpl_->remote_disconnect(); }
void socket::start(uint16_t port) { pimpl_->server_start(port ? port : 8080); }
void socket::stop() { pimpl_->server_stop(); }
void socket::send_to_remote(const uuid& rid, std::shared_ptr<packet> pkt) { pimpl_->server_send(rid, pkt); }
void socket::send_to_remotes(std::shared_ptr<packet> pkt) { pimpl_->server_send_every(pkt); }
void socket::tick() { pimpl_->tick(this); }
void socket::hold_alive(const uuid& id) { pimpl_->hold_alive(id); }

static socket server_s;
static socket remote_s;
socket* socket::server = &server_s;
socket* socket::remote = &remote_s;

uint16_t gen_tcp_port_() {
    asio::io_context ioc;
    tcp::socket s(ioc);

    try {
        s.open(tcp::v4());
        s.bind(tcp::endpoint(tcp::v4(), 0));
    } catch (const std::exception& e) {
        print_throw(log_level::fatal, e.what());
    }
    return s.local_endpoint().port();
}

uint16_t gen_udp_port_() {
    asio::io_context ioc;
    udp::socket s(ioc);

    try {
        s.open(udp::v4());
        s.bind(udp::endpoint(udp::v4(), 0));
    } catch (const std::exception& e) {
        print_throw(log_level::fatal, e.what());
    }
    return s.local_endpoint().port();
}

}  // namespace arc