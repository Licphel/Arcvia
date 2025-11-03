#include "audio/device.h"
#include "core/io.h"
#include "lua/lua.h"

namespace arc {

void lua_bind_tcodec(lua_state& lua) {
    auto _n = lua_make_table();

    lua["arc"]["codec"] = _n;
}

}  // namespace arc