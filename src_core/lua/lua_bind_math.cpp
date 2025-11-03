#include <memory>

#include "core/math.h"
#include "core/rand.h"
#include "lua.h"
#include "lua/lua.h"

namespace arc {

void lua_bind_math(lua_state& lua) {
    auto _n = lua_make_table();

    // vec2
    auto vec2_type = lua_new_usertype<vec2>(_n, "vec2", lua_constructors<vec2(), vec2(double, double)>());
    vec2_type["x"] = &vec2::x;
    vec2_type["y"] = &vec2::y;
    vec2_type["length"] = &vec2::length;
    vec2_type["length_powered"] = &vec2::length_powered;
    vec2_type["normal"] = &vec2::normal;
    vec2_type["from"] = &vec2::from;
    vec2_type["dot"] = &vec2::dot;
    vec2_type["__add"] = lua_combine(TYPE_OP_C_(vec2, vec2, const vec2&, +), TYPE_OP_C_(vec2, vec2, double, +));
    vec2_type["__sub"] = lua_combine(TYPE_OP_C_(vec2, vec2, const vec2&, -), TYPE_OP_C_(vec2, vec2, double, -));
    vec2_type["__mul"] = lua_combine(TYPE_OP_C_(vec2, vec2, const vec2&, *), TYPE_OP_C_(vec2, vec2, double, *));
    vec2_type["__div"] = lua_combine(TYPE_OP_C_(vec2, vec2, const vec2&, /), TYPE_OP_C_(vec2, vec2, double, /));

    // vec3
    auto vec3_type = lua_new_usertype<vec3>(_n, "vec3", lua_constructors<vec3(), vec3(double, double, double)>());
    vec3_type["x"] = &vec3::x;
    vec3_type["y"] = &vec3::y;
    vec3_type["z"] = &vec3::z;
    vec3_type["length"] = &vec3::length;
    vec3_type["length_powered"] = &vec3::length_powered;
    vec3_type["normal"] = &vec3::normal;
    vec3_type["dot"] = &vec3::dot;
    vec3_type["__add"] = lua_combine(TYPE_OP_C_(vec3, vec3, const vec3&, +), TYPE_OP_C_(vec3, vec3, double, +));
    vec3_type["__sub"] = lua_combine(TYPE_OP_C_(vec3, vec3, const vec3&, -), TYPE_OP_C_(vec3, vec3, double, -));
    vec3_type["__mul"] = lua_combine(TYPE_OP_C_(vec3, vec3, const vec3&, *), TYPE_OP_C_(vec3, vec3, double, *));
    vec3_type["__div"] = lua_combine(TYPE_OP_C_(vec3, vec3, const vec3&, /), TYPE_OP_C_(vec3, vec3, double, /));

    // quad
    auto quad_type =
        lua_new_usertype<quad>(_n, "quad", lua_constructors<vec3(), vec3(double, double, double, double)>());
    quad_type["x"] = &quad::x;
    quad_type["y"] = &quad::y;
    quad_type["width"] = &quad::width;
    quad_type["height"] = &quad::height;
    quad_type["corner"] = &quad::corner;
    quad_type["center"] = &quad::center;
    quad_type["intersection_of"] = &quad::intersection_of;
    quad_type["intersect"] = &quad::intersect;
    quad_type["contain"] = &quad::contain;
    quad_type["corner_x"] = &quad::corner_x;
    quad_type["corner_y"] = &quad::corner_y;
    quad_type["center_x"] = &quad::center_x;
    quad_type["center_y"] = &quad::center_y;
    quad_type["prom_x"] = &quad::prom_x;
    quad_type["prom_y"] = &quad::prom_y;
    quad_type["translate"] = &quad::translate;
    quad_type["locate_center"] = &quad::locate_center;
    quad_type["locate_corner"] = &quad::locate_corner;
    quad_type["resize"] = &quad::resize;
    quad_type["scale"] = &quad::scale;
    quad_type["inflate"] = &quad::inflate;
    quad_type["area"] = &quad::area;

    // transform
    auto trs_type = lua_new_usertype<transform>(_n, "transform", lua_constructors<transform()>());
    trs_type["m00"] = &transform::m00;
    trs_type["m01"] = &transform::m01;
    trs_type["m02"] = &transform::m02;
    trs_type["m10"] = &transform::m10;
    trs_type["m11"] = &transform::m11;
    trs_type["m12"] = &transform::m12;
    trs_type["translate"] = &transform::translate;
    trs_type["rotate"] = &transform::rotate;
    trs_type["shear"] = &transform::shear;
    trs_type["scale"] = &transform::scale;
    trs_type["det"] = &transform::det;
    trs_type["invert"] = &transform::invert;
    trs_type["scale"] = &transform::scale;
    trs_type["orthographic"] = &transform::orthographic;
    trs_type["mul"] = &transform::mul;
    // lua cannot handle reference.
    // trs_type["apply"] = lua_combine(TYPE_FN_C_(transform, void, apply, vec2&), TYPE_FN_C_(transform, void, apply,
    // float&, float&));

    // random
    auto rand_type = lua_new_usertype<random>(_n, "random", lua_native);
    _n["random"]["rg"] = random::rg;
    rand_type["make"] =
        lua_combine(FN_(random, std::shared_ptr<random>, make), FN_(random, std::shared_ptr<random>, make, long));
    rand_type["next"] = lua_combine(TYPE_FN_(random, double, next), TYPE_FN_(random, double, next, double, double));

    lua["arc"]["math"] = _n;
}

}  // namespace arc