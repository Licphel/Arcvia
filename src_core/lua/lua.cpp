#include "lua/lua.h"

#include "core/io.h"
#include "core/loc.h"

using namespace sol;

namespace arc {

static lua_state lua;

lua_state& lua_get_gstate() { return lua; }

int __c_lua_load(lua_State* L) {
    location module_id = location(luaL_checkstring(L, 1));
    path path_ = module_id.find_path();

    if (!path_.exists() || path_.judge() != path_type::file) return 0;

    std::string code = io::read_str(path_);

    if (luaL_loadbuffer(L, code.data(), code.size(), path_.strp.c_str()) != LUA_OK) return lua_error(L);
    return 1;
}

void lua_make_state() {
    lua.open_libraries(lib::base, lib::bit32, lib::coroutine, lib::count, lib::debug, lib::io, lib::math, lib::os,
                       lib::package, lib::string, lib::table, lib::utf8);
#ifdef ARC_USE_LUA_JIT_
    lua.open_libraries(lib::ffi, lib::jit);

    if (lua["jit"].valid())
        print(log_level::info, "lua-jit is enabled.");
    else
        print(log_level::info, "lua-jit is disabled.");

    lua.set_exception_handler([](lua_State* L, sol::optional<const std::exception&> ex, sol::string_view desc) {
        std::string error_msg;
        if (ex)
            error_msg = std::string("c++ exception: ") + ex->what();
        else
            error_msg = std::string("lua error: ") + std::string(desc);

        print(log_level::warn, error_msg);
        lua_pushstring(L, error_msg.c_str());
        return 1;
    });
#endif
    // add package researcher
    lua_table searchers = lua["package"]["searchers"];
    // this is a lua-c-function (not packed by sol2)
    // stupid lua, I cannot insert my function to the beginning. why?
    searchers.add(__c_lua_load);
}

lua_table lua_make_table() { return lua.create_table(); }

void lua_eval(const std::string& code) { lua.script(code); }

void lua_eval(lua_program& code) { code(); }

lua_program lua_compile(const std::string& code) { return lua.load(code); }

extern void lua_bind_gfx(lua_state& lua);
extern void lua_bind_math(lua_state& lua);
extern void lua_bind_ecs(lua_state& lua);
extern void lua_bind_asset(lua_state& lua);
extern void lua_bind_base(lua_state& lua);
extern void lua_bind_tcodec(lua_state& lua);
extern void lua_bind_net(lua_state& lua);

void lua_bind_modules() {
    lua["arc"] = lua_make_table();

    lua_bind_base(lua);
    lua_bind_math(lua);
    lua_bind_ecs(lua);
    lua_bind_asset(lua);
    lua_bind_gfx(lua);
    lua_bind_tcodec(lua);
    lua_bind_net(lua);
}

}  // namespace arc
