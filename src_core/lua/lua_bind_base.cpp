#include "core/buffer.h"
#include "core/io.h"
#include "core/uuid.h"
#include "lua/lua.h"

namespace arc {

void lua_bind_base(lua_state& lua) {
    auto _n = lua_make_table();

    // path
    auto pathype = lua_new_usertype<path>(_n, "path", lua_native);
    pathype["strp"] = &path::strp;
    pathype["file_name"] = &path::file_name;
    pathype["file_format"] = &path::file_format;

    // path op
    _n["open"] = &path::open;
    _n["open_local"] = &path::open_local;
    _n["exists"] = &path::exists;
    _n["del"] = &path::del;
    _n["parent"] = &path::parent;
    _n["exists"] = &path::exists;
    _n["rename"] = &path::rename;
    _n["mkdirs"] = &path::mkdirs;
    _n["judge"] = &path::judge;
    _n["sub_dirs"] = &path::sub_dirs;
    _n["sub_files"] = &path::sub_files;
    _n["recurse_files"] = &path::recurse_files;
    _n["execution_path"] = &path::execution_path;
    _n["read_str"] = &io::read_str;
    _n["write_str"] = &io::write_str;
    _n["read_bytes"] =
        lua_combine([](const path& path_) { return byte_buf(io::read_bytes(path_)); },
                    [](const path& path_, compression_level lvl) { return byte_buf(io::read_bytes(path_, lvl)); });
    _n["write_bytes"] = [](const path& path_, const byte_buf& buf) { return io::write_bytes(path_, buf.to_vector()); };

    // compression level
    auto cmplvl_type = lua_make_table();
    cmplvl_type["NO"] = compression_level::no;
    cmplvl_type["smallest"] = compression_level::smallest;
    cmplvl_type["optimal"] = compression_level::optimal;
    cmplvl_type["fastest"] = compression_level::fastest;
    cmplvl_type["raw_read"] = compression_level::raw_read;
    cmplvl_type["dcmp_read"] = compression_level::dcmp_read;
    _n["compression_level"] = cmplvl_type;

    // byte_buf
    auto buf_type = lua_new_usertype<byte_buf>(
        _n, "byte_buf", lua_constructors<byte_buf(), byte_buf(size_t), byte_buf(const byte_buf&)>());
    buf_type["size"] = &byte_buf::size;
    buf_type["capacity"] = &byte_buf::capacity;
    buf_type["free_bytes"] = &byte_buf::free_bytes;
    buf_type["readable_bytes"] = &byte_buf::readable_bytes;
    buf_type["is_empty"] = &byte_buf::is_empty;
    buf_type["read_pos"] = &byte_buf::read_pos;
    buf_type["write_pos"] = &byte_buf::write_pos;
    buf_type["clear"] = &byte_buf::clear;
    buf_type["reserve"] = &byte_buf::reserve;
    buf_type["resize"] = &byte_buf::resize;
    buf_type["rewind"] = &byte_buf::rewind;
    buf_type["compact"] = &byte_buf::compact;
    buf_type["skip"] = &byte_buf::skip;
    buf_type["set_read_pos"] = &byte_buf::set_read_pos;
    buf_type["set_write_pos"] = &byte_buf::set_write_pos;
    buf_type["read_advance"] = &byte_buf::read_advance;
    buf_type["to_vector"] = &byte_buf::to_vector;
    buf_type["write_byte"] = [](byte_buf& self, uint8_t value) { self.write<uint8_t>(value); };
    buf_type["write_int"] = [](byte_buf& self, int value) { self.write<int>(value); };
    buf_type["write_uint"] = [](byte_buf& self, unsigned int value) { self.write<unsigned int>(value); };
    buf_type["write_long"] = [](byte_buf& self, long value) { self.write<long>(value); };
    buf_type["write_ulong"] = [](byte_buf& self, unsigned long value) { self.write<unsigned long>(value); };
    buf_type["write_float"] = [](byte_buf& self, float value) { self.write<float>(value); };
    buf_type["write_double"] = [](byte_buf& self, double value) { self.write<double>(value); };
    buf_type["write_bool"] = [](byte_buf& self, bool value) { self.write<bool>(value); };
    buf_type["write_short"] = [](byte_buf& self, short value) { self.write<short>(value); };
    buf_type["write_ushort"] = [](byte_buf& self, unsigned short value) { self.write<unsigned short>(value); };
    buf_type["write_byte_buf"] = [](byte_buf& self, const byte_buf& value) { self.write<byte_buf>(value); };
    buf_type["write_uuid"] = [](byte_buf& self, const uuid& value) { self.write<uuid>(value); };
    buf_type["write_string"] = [](byte_buf& self, const std::string& value) { self.write<std::string>(value); };

    buf_type["read_byte"] = [](byte_buf& self) { return self.read<uint8_t>(); };
    buf_type["read_int"] = [](byte_buf& self) { return self.read<int>(); };
    buf_type["read_uint"] = [](byte_buf& self) { return self.read<unsigned int>(); };
    buf_type["read_long"] = [](byte_buf& self) { return self.read<long>(); };
    buf_type["read_ulong"] = [](byte_buf& self) { return self.read<unsigned long>(); };
    buf_type["read_float"] = [](byte_buf& self) { return self.read<float>(); };
    buf_type["read_double"] = [](byte_buf& self) { return self.read<double>(); };
    buf_type["read_bool"] = [](byte_buf& self) { return self.read<bool>(); };
    buf_type["read_short"] = [](byte_buf& self) { return self.read<short>(); };
    buf_type["read_ushort"] = [](byte_buf& self) { return self.read<unsigned short>(); };
    buf_type["read_byte_buf"] = [](byte_buf& self) { return self.read<byte_buf>(); };
    buf_type["read_uuid"] = [](byte_buf& self) { return self.read<uuid>(); };
    buf_type["read_string"] = [](byte_buf& self) { return self.read<std::string>(); };

    auto uuid_type = lua_new_usertype<uuid>(_n, "uuid", lua_native);
    uuid_type["make"] = &uuid::make;
    uuid_type["empty"] = &uuid::empty;
    uuid_type["hash_"] = &uuid::hash_;
    uuid_type["__eq"] = &uuid::operator==;
    uuid_type["__lt"] = &uuid::operator<;

    lua["arc"]["io"] = _n;
}

}  // namespace arc