#pragma once
#include <functional>
#include <unordered_map>
#include <vector>

#include "core/buffer.h"
#include "core/uuid.h"

namespace arc {

enum class ecs_component_sync_mode : uint8_t { none, predict, auth };

struct ecs_pool_terased {
    int index;

    virtual ~ecs_pool_terased() = default;

    virtual void add_raw(const uuid& e, const void* src) = 0;
    virtual void* get_raw(const uuid& e) = 0;
    virtual void remove(const uuid& e) = 0;
    virtual void write(const uuid& e, byte_buf&) = 0;
    virtual void read(const uuid& e, byte_buf&) = 0;
};

template <typename T>
struct ecs_pool : ecs_pool_terased {
    struct slot_ {
        T data{};
        bool alive = false;
        size_t idx = 0;
    };

    std::unordered_map<uuid, slot_> sparse;
    std::vector<uuid> dense;
    std::vector<T> data;

    T* add(const uuid& e, const T& v) {
        auto& s = sparse[e];
        if (s.alive) return nullptr;
        s.alive = true;
        s.data = v;
        s.idx = dense.size();
        dense.push_back(e);
        data.push_back(v);
        return &data.back();
    }

    T* get(const uuid& e) {
        auto it = sparse.find(e);
        if (it == sparse.end()) return nullptr;
        auto& s = it->second;
        return (s.alive) ? &data[it->second.idx] : nullptr;
    }

    void remove(const uuid& e) override {
        auto it = sparse.find(e);
        if (it == sparse.end() || !it->second.alive) return;
        // move the last slot to the removed one, ensuring no empty slots.
        size_t idx = it->second.idx;
        uuid tail = dense.back();
        dense[idx] = tail;
        data[idx] = data.back();
        sparse[tail].idx = idx;
        dense.pop_back();
        data.pop_back();
        sparse.erase(it);
    }

    void clear() {
        sparse.clear();
        dense.clear();
        data.clear();
    }

    void each(const std::function<void(const uuid& ref, T& cmp)>& f) {
        for (size_t i = 0; i < dense.size(); ++i) f(dense[i], data[i]);
    }

    void write(const uuid& e, arc::byte_buf& buf) override {
        if (T* p = get(e)) p->write(buf);
    }

    void read(const uuid& e, arc::byte_buf& buf) override {
        if (T* p = get(e)) p->read(buf);
    }

    void add_raw(const uuid& e, const void* src) override {
        // recover T
        add(e, *static_cast<const T*>(src));
    }

    void* get_raw(const uuid& e) override { return get(e); }
};

#define ARC_ECS_PHASE_COUNT 3
enum class ecs_phase : uint8_t { pre, common, post };

/*
an acceptable ecs component is like:

struct position {
    double x;
    double y;

    void write/read(const byte_buf& buf) {
        ...
    }
};
*/

}  // namespace arc