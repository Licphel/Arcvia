#pragma once
#include <any>
#include <functional>
#include <stack>
#include <string>
#include <unordered_map>

#include "core/io.h"
#include "core/loc.h"
#include "core/multic.h"

namespace arc {

std::unordered_map<location, std::any>& get_resource_map_();

// get a loaded resource by its id.
template <typename T>
T fetch(const location& loc) {
    const auto& m = get_resource_map_();
    auto it = m.find(loc);
    bool cf = it != m.end() && it->second.has_value();
    return cf ? std::any_cast<T>(it->second) : std::decay_t<T>{};
}

enum class loader_equip_m { png_tex, png_img, txt, fnt, shd, wav, lua };

struct scan_loader {
    using proc_strategy = multicall<void(const path& path_, const location& loc)>;

    std::string scope;
    path root;
    double progress;
    int done_tcount_;
    int total_tcount_;
    std::unordered_map<std::string, proc_strategy> process_strategy_map;
    std::stack<std::function<void()>> tasks;
    std::vector<std::shared_ptr<scan_loader>> subloaders;
    multicall<void()> event_on_start;
    multicall<void()> event_on_end;
    bool start_called_;
    bool end_called_;

    ~scan_loader();

    void scan(const path& path_root);
    void add_sub(std::shared_ptr<scan_loader> subloader);
    // run a task in the queue.
    // you may need to check the #progress to see if all tasks are done.
    void next();
    void free_node(const location& loc);
    void free();

    // add a built-in loader behavior to the loader.
    void add_equipment(loader_equip_m equipment);

    static std::shared_ptr<scan_loader> make(const std::string& scope, const path& root);
};

}  // namespace arc
