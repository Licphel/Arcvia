#include "core/load.h"

#include "audio/device.h"
#include "core/io.h"
#include "core/loc.h"
#include "gfx/image.h"

using namespace arc;
using namespace arc;

namespace arc {

std::unordered_map<location, std::any> resource_map_;

std::unordered_map<location, std::any>& get_resource_map_() { return resource_map_; }

void scan_loader::scan(const path& path_root) {
    for (const path& path_ : path_root.recurse_files()) {
        if (path_.judge() == path_type::file) {
            location loc = location(scope, path_ - root);
            std::string fmt = path_.file_format();

            if (process_strategy_map.find(fmt) != process_strategy_map.end()) {
                proc_strategy sttg = process_strategy_map[fmt];
                tasks.push([sttg, path_, loc]() { sttg(path_, loc); });
                total_tcount_++;
            }
        }
    }
}

void scan_loader::add_sub(std::shared_ptr<scan_loader> subloader) {
    subloaders.push_back(subloader);
    total_tcount_ += subloader->total_tcount_;
}

void scan_loader::next() {
    if (end_called_) return;

    if (!start_called_) {
        event_on_start();
        start_called_ = true;
    }

    if (!tasks.empty()) {
        auto fn = tasks.top();
        tasks.pop();
        fn();
        done_tcount_++;
        progress = static_cast<double>(done_tcount_) / static_cast<double>(total_tcount_);
    } else {
        progress = 1;

        bool can_find = false;
        for (auto& sub : subloaders) {
            if (sub->progress < 1) {
                sub->next();
                can_find = true;
                break;
            }
        }
        if (!can_find) {
            if (!end_called_) {
                event_on_end();
                free();
                end_called_ = true;
            }
        }
    }
}

void scan_loader::free_node(const location& loc) { resource_map_.erase(loc); }

void scan_loader::free() { resource_map_.clear(); }

scan_loader::~scan_loader() { free(); }

std::shared_ptr<scan_loader> scan_loader::make(const std::string& scope, const path& root) {
    std::shared_ptr<scan_loader> lptr = std::make_shared<scan_loader>();
    lptr->scope = scope;
    lptr->root = root;
    return lptr;
}

void scan_loader::add_equipment(loader_equip_m equipment) {
    switch (equipment) {
        case loader_equip_m::png_tex:
            process_strategy_map[".png"] += [](const path& path_, const location& loc) {
                std::shared_ptr<image> img = image::load(path_);
                std::shared_ptr<texture> tex = texture::make(img);
                resource_map_[loc] = std::any(tex);
            };
            break;
        case loader_equip_m::png_img:
            process_strategy_map[".png"] += [](const path& path_, const location& loc) {
                std::shared_ptr<image> img = image::load(path_);
                resource_map_[loc] = std::any(img);
            };
            break;
        case loader_equip_m::txt:
            process_strategy_map[".txt"] +=
                [](const path& path_, const location& loc) { resource_map_[loc] = std::any(io::read_str(path_)); };
            break;
        case loader_equip_m::wav:
            process_strategy_map[".wav"] += [](const path& path_, const location& loc) {
                std::shared_ptr<track> track = track::load(path_);
                resource_map_[loc] = std::any(track);
            };
            break;
        case loader_equip_m::fnt:
            break;
        case loader_equip_m::lua:
        case loader_equip_m::shd:
            break;
    }
}

}  // namespace arc
