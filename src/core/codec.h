#pragma once
#include <core/buffer.h>
#include <core/def.h>
#include <core/io.h>
#include <core/log.h>

#include <any>
#include <cstdint>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace arc {

struct codec_map;
struct codec_vector;

enum class codec_type_ : uint8_t { b, n, s, m, v, f, i, u };

struct variant_ {
    codec_type_ type;
    std::any anyv_;

    template <typename T>
    static variant_ make(const T& v);

    template <typename T>
    T cast() const;
};

struct codec_map {
    std::unordered_map<std::string, variant_> data;

    size_t size() const { return data.size(); }

    template <typename T>
    T get(const std::string& key, const T& def = T()) const {
        auto it = data.find(key);
        if (it == data.end()) return def;
        return it->second.cast<T>();
    }

    bool has(const std::string& key) const { return data.find(key) != data.end(); }

    template <typename T>
    void set(const std::string& key, const T& val) {
        data[key] = variant_::make(val);
    }

    // read a script-form binary map (like json, but not the same).
    static codec_map load_lang(const path& path_);
    static codec_map load(byte_buf& v);
    static codec_map load(const path& path_);

    byte_buf write() const;
    void write(const path& path_) const;
};

struct codec_vector {
    std::vector<variant_> data;

    size_t size() const { return data.size(); }

    template <typename T>
    T get(int i, const T& def = T()) const {
        if (i < 0 || i >= data.size()) return def;
        return data[i].cast<T>();
    }

    template <typename T>
    void set(int i, const T& val) {
        data[i] = variant_::make(val);
    }

    template <typename T>
    void push(const T& val) {
        data.push_back(variant_::make(val));
    }
};

template <typename T>
variant_ variant_::make(const T& v) {
    using U = std::decay_t<T>;

    if constexpr (std::is_same_v<T, bool>)
        return {codec_type_::b, std::any(v)};
    else if constexpr (std::is_arithmetic_v<T>)
        return {codec_type_::n, std::any(static_cast<double>(v))};
    // it's tricky to check string types, so we just check if it's constructible.
    // I've tried const char* & char[], but they don't cover all cases.
    else if constexpr (std::is_constructible_v<std::string, U>)
        return {codec_type_::s, std::any(std::string(v))};
    else if constexpr (std::is_same_v<codec_map, U>)
        return {codec_type_::m, std::any(v)};
    else if constexpr (std::is_same_v<codec_vector, U>)
        return {codec_type_::v, std::any(v)};
    else if constexpr (std::is_same_v<byte_buf, U>)
        return {codec_type_::f, std::any(v)};
    else if constexpr (std::is_same_v<uuid, U>) {
        return {codec_type_::u, std::any(v)};
    }

    print_throw(log_level::fatal, "unsupported type.");
}

template <typename T>
T variant_::cast() const {
    switch (type) {
        case codec_type_::b:
            if constexpr (std::is_convertible_v<bool, T>) return std::any_cast<bool>(anyv_);
        case codec_type_::n:
            if constexpr (std::is_arithmetic_v<T>) return static_cast<T>(std::any_cast<double>(anyv_));
        case codec_type_::s:
            if constexpr (std::is_convertible_v<std::string, T>) return std::any_cast<std::string>(anyv_);
        case codec_type_::m:
            if constexpr (std::is_convertible_v<codec_map, T>) return std::any_cast<codec_map>(anyv_);
        case codec_type_::v:
            if constexpr (std::is_convertible_v<codec_vector, T>) return std::any_cast<codec_vector>(anyv_);
        case codec_type_::f:
            if constexpr (std::is_convertible_v<byte_buf, T>) return std::any_cast<byte_buf>(anyv_);
        case codec_type_::u:
            if constexpr (std::is_convertible_v<uuid, T>) return std::any_cast<uuid>(anyv_);
        default:
            print_throw(log_level::fatal, "not convertible.");
    }
    return T{};
}

}  // namespace arc