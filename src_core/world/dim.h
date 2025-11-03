#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

#include "block.h"
#include "core/ecs.h"
#include "core/uuid.h"
#include "render/light.h"
#include "world/entity.h"
#include "world/liquid.h"
#include "world/chunk.h"
#include "world/pos.h"

namespace arc {

struct dimension;

struct componentable_dimension_ {
    using sysfn_ = std::function<void(dimension&)>;

    std::unordered_map<std::string, std::shared_ptr<ecs_pool_terased>> ecs_pools_;
    std::vector<sysfn_> ecs_syses_[ARC_ECS_PHASE_COUNT];

    uuid make_entity() {
        uuid ref = uuid::make();
        return ref;
    }

    void destroy_entity(const uuid& e) {
        for (auto& kv : ecs_pools_) kv.second->remove(e);
    }

    template <typename T>
    ecs_pool<T>* get_pool(const std::string& k) {
        auto it = ecs_pools_.find(k);
        if (it == ecs_pools_.end()) {
            ecs_pools_[k] = std::make_shared<ecs_pool<T>>();
            it = ecs_pools_.find(k);
        }
        return static_cast<ecs_pool<T>*>(it->second.get());
    }

    template <typename T>
    void add_component(const std::string& k, const uuid& e, const T& cmp) {
        get_pool<T>(k)->add_raw(e, &cmp);
    }

    // never keep a component object!
    template <typename T>
    T* get_component(const std::string& k, const uuid& e) {
        return get_pool<T>(k)->get(e);
    }

    template <typename T>
    void remove_component(const std::string& k, const uuid& e) {
        get_pool<T>(k)->remove(e);
    }

    template <typename F>
    void add_system(ecs_phase ph, F&& f) {
        ecs_syses_[static_cast<int>(ph)].emplace_back(std::forward<F>(f));
    }

    void tick_phase(ecs_phase ph) {
        for (auto& sys : ecs_syses_[static_cast<int>(ph)]) sys((dimension&)*this);
    }

    void tick_systems() {
        tick_phase(ecs_phase::pre);
        tick_phase(ecs_phase::common);
        tick_phase(ecs_phase::post);
    }

    template <typename T>
    void each(const std::string& k, const std::function<void(dimension& lvl, const uuid& ref, T& cmp)>& f) {
        get_pool<T>(k)->each([this, f](const uuid& ref, T& cmp) { f((dimension&)*this, ref, cmp); });
    }
};

enum class find_chunk_flag { no, mk_cache_if_absent, cache };

struct dimension : componentable_dimension_ {
    static const int sea_level = 0;

    std::mutex chunkop_mutex_;
    std::mutex chunkcop_mutex_;
    std::unordered_map<pos2i, std::shared_ptr<chunk>> chunk_map;
    std::unordered_map<pos2i, std::shared_ptr<chunk>> chunk_cache_map;
    std::unique_ptr<light_engine> light_executor = nullptr;
    std::unordered_map<uuid, std::shared_ptr<entity>> entities;
    bool server;
    bool remote;
    uint32_t ticks;

    void init();
    void tick();

    obs<chunk> find_chunk(const pos2i& pos, find_chunk_flag flag = find_chunk_flag::no);
    obs<chunk> find_chunk_by_block(const pos2i& pos, find_chunk_flag flag = find_chunk_flag::no);
    std::shared_ptr<chunk> consume_chunk_cache(const pos2i& pos);
    void set_chunk(const pos2i& pos, std::shared_ptr<chunk> chunk_);
    void set_chunk_cache(const pos2i& pos, std::shared_ptr<chunk> chunk_);

    block_behavior* find_block(const pos2i& pos);
    void set_block(block_behavior* block, const pos2i& pos, set_block_flag flag = set_block_flag::no);
    block_behavior* find_back_block(const pos2i& pos);
    void set_back_block(block_behavior* block, const pos2i& pos, set_block_flag flag = set_block_flag::no);
    obs<block_entity> find_block_entity(const pos2i& pos);
    void set_block_entity(std::shared_ptr<block_entity> ent, const pos2i& pos);
    liquid_stack find_liquid_stack(const pos2i& pos);
    void set_liquid_stack(const liquid_stack& s, const pos2i& pos);
    obs<codec_map> find_place_cdmap(const pos2i& pos);
    obs<codec_map> ensure_place_cdmap(const pos2i& pos);
    void spawn_entity(std::shared_ptr<entity> e);
    obs<entity> find_entity(const uuid& id);
    void remove_entity(const uuid& id);
};

}  // namespace arc