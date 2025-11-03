#include "audio/device.h"
#include "core/load.h"
#include "core/loc.h"
#include "gfx/font.h"
#include "gfx/image.h"
#include "gfx/shader.h"
#include "lua/lua.h"

namespace arc {

void lua_bind_asset(lua_state& lua) {
    auto _n = lua_make_table();

    // unique_id
    auto uid_type = lua_new_usertype<location>(
        _n, "unique_id",
        lua_constructors<location(const std::string&), location(const std::string&, const std::string&)>());
    uid_type["concat"] = &location::concat;
    uid_type["scope"] = &location::scope;
    uid_type["key"] = &location::key;
    uid_type["hash_"] = &location::hash_;
    uid_type["find_path"] = &location::find_path;
    uid_type["__eq"] = &location::operator==;
    uid_type["__lt"] = &location::operator<;

    // asset_loader
    auto asset_loaderype = lua_new_usertype<scan_loader>(_n, "asset_loader", lua_native);
    asset_loaderype["scan"] = &scan_loader::scan;
    asset_loaderype["next"] = &scan_loader::next;
    asset_loaderype["add_sub"] = &scan_loader::add_sub;
    asset_loaderype["add_strategy"] = [](scan_loader& self, const std::string& fmt, const lua_function& f) {
        self.process_strategy_map[fmt] +=
            [f](const path& path_, const location& loc) { get_resource_map_()[loc] = lua_protected_call(f, path_); };
    };
    asset_loaderype["make"] = &scan_loader::make;

    // asset_mapping
    _n["has"] =
        lua_combine([](const location& loc) { return get_resource_map_().find(loc) != get_resource_map_().end(); },
                    [](const std::string& loc) { return get_resource_map_().find(loc) != get_resource_map_().end(); });
    _n["texture"] = lua_combine([](const location& loc) { return fetch<std::shared_ptr<texture>>(loc); },
                                [](const std::string& loc) { return fetch<std::shared_ptr<texture>>(loc); });
    _n["track"] = lua_combine([](const location& loc) { return fetch<std::shared_ptr<track>>(loc); },
                              [](const std::string& loc) { return fetch<std::shared_ptr<track>>(loc); });
    _n["font"] = lua_combine([](const location& loc) { return fetch<std::shared_ptr<font>>(loc); },
                             [](const std::string& loc) { return fetch<std::shared_ptr<font>>(loc); });
    _n["image"] = lua_combine([](const location& loc) { return fetch<std::shared_ptr<image>>(loc); },
                              [](const std::string& loc) { return fetch<std::shared_ptr<image>>(loc); });
    _n["program"] = lua_combine([](const location& loc) { return fetch<std::shared_ptr<program>>(loc); },
                                [](const std::string& loc) { return fetch<std::shared_ptr<program>>(loc); });
    _n["text"] = lua_combine([](const location& loc) { return fetch<std::string>(loc); },
                             [](const std::string& loc) { return fetch<std::string>(loc); });

    lua["arc"]["asset"] = _n;
}

}  // namespace arc