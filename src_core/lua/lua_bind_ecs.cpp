#include "core/ecs.h"
#include "lua/lua.h"
#include "world/dim.h"

namespace arc {

struct lua_ecs_component {
    lua_table data;

    lua_ecs_component() { data = lua_get_gstate().create_table(); }
    lua_ecs_component(const lua_table& tb) : data(tb) {}

    void write(byte_buf& buf) {}
    void read(byte_buf& buf) {}
};

void lua_bind_ecs(lua_state& lua) {
    auto _n = lua_make_table();

    // component
    auto cmp_type = lua_new_usertype<lua_ecs_component>(
        _n, "component_", lua_constructors<lua_ecs_component(), lua_ecs_component(lua_table)>());
    cmp_type["data"] = &lua_ecs_component::data;
    cmp_type["write"] = &lua_ecs_component::write;
    cmp_type["read"] = &lua_ecs_component::read;

    // level
    auto dim_type = lua_new_usertype<dimension>(_n, "dimension", lua_native);
    dim_type["add_component"] = [](dimension& self, const std::string& k, const uuid& e, const lua_table& cmp) {
        self.add_component<lua_ecs_component>(k, e, lua_ecs_component(cmp));
    };
    dim_type["get_component"] = [](dimension& self, const std::string& k, const uuid& e) {
        auto* ptr = self.get_component<lua_ecs_component>(k, e);
        return ptr ? ptr->data : lua_make_table();
    };
    dim_type["has_component"] = [](dimension& self, const std::string& k, const uuid& e) {
        return self.get_component<lua_ecs_component>(k, e) != nullptr;
    };
    dim_type["remove_component"] = [](dimension& self, const std::string& k, const uuid& e) {
        self.remove_component<lua_ecs_component>(k, e);
    };
    dim_type["make_entity"] = [](dimension& self) { return self.make_entity(); };
    dim_type["destroy_entity"] = [](dimension& self, const uuid& e) { self.destroy_entity(e); };
    dim_type["add_system"] = [](dimension& self, ecs_phase ph, const lua_function& f) {
        self.add_system(ph, [f](dimension& lvl) { lua_protected_call(f, lvl); });
    };
    dim_type["each_components"] = [](dimension& self, const std::string& k, const lua_function& f) {
        self.each<lua_ecs_component>(k, [f](dimension& lvl, const uuid& ref, lua_ecs_component& cmp) {
            lua_protected_call(f, lvl, ref, cmp.data);
        });
    };

    // ecs_phase
    auto table_sys_ph = lua_make_table();
    table_sys_ph["pre"] = ecs_phase::pre;
    table_sys_ph["common"] = ecs_phase::common;
    table_sys_ph["post"] = ecs_phase::post;
    _n["phase"] = table_sys_ph;

    lua["arc"]["ecs"] = _n;
}

}  // namespace arc