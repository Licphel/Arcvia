#pragma once

#include <sol/sol.hpp>

#include "core/log.h"

#define ARC_USE_LUA_JIT_
#define TYPE_OP_C_(type, returnType, withType, op) static_cast<returnType (type::*)(withType) const>(&type::operator op)
#define TYPE_OP_(type, returnType, withType, op) static_cast<returnType (type::*)(withType)>(&type::operator op)
#define TYPE_FN_C_(type, returnType, fnName, ...) static_cast<returnType (type::*)(__VA_ARGS__) const>(&type::fnName)
#define TYPE_FN_(type, returnType, fnName, ...) static_cast<returnType (type::*)(__VA_ARGS__)>(&type::fnName)
#define FN_(type, returnType, fnName, ...) static_cast<returnType (*)(__VA_ARGS__)>(&type::fnName)

namespace arc {

using lua_table = sol::table;
using lua_state = sol::state;
using lua_vargs = sol::variadic_args;
using lua_program = sol::load_result;
using lua_function = sol::function;
using lua_object = sol::object;
template <typename T>
using lua_type = sol::usertype<T>;
template <typename... Args>
using lua_constructors = sol::constructors<Args...>;
inline static auto lua_native = sol::no_constructor;

lua_state& lua_get_gstate();
void lua_make_state();
void lua_bind_modules();
void lua_eval(const std::string& code);
void lua_eval(lua_program& code);
lua_program lua_compile(const std::string& code);
void lua_free();

template <typename T>
void lua_push(const std::string& key, const T& v) {
    lua_get_gstate()[key] = v;
}

template <typename T>
T lua_get(const std::string& key) {
    return lua_get_gstate()[key];
}

template <typename... Args>
lua_object lua_protected_call(const lua_function& func, Args&&... args) {
    auto result = func(std::forward<Args>(args)...);

    if (result.valid()) return result;

    sol::error err = result;
    print_throw(log_level::fatal, err.what());
    return result;
}

template <typename... Args>
decltype(auto) lua_combine(Args&&... args) {
    return sol::overload(std::forward<Args>(args)...);
}

template <typename T>
lua_object lua_ref(T&& ptr) {
    return sol::make_object(lua_get_gstate(), std::ref(ptr));
}

template <typename Class, typename... Args>
lua_type<Class> lua_new_usertype(lua_table& tb, const std::string& name, Args&&... args) {
    return tb.new_usertype<Class>(name, sol::call_constructor, std::forward<Args>(args)...);
}

lua_table lua_make_table();

template <typename T>
lua_table lua_vector(const std::vector<T> vec) {
    return sol::as_table(vec);
}

}  // namespace arc