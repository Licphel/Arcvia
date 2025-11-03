#pragma once
#include <string>

#include "core/io.h"

namespace arc {

struct location {
    std::string scope;
    std::string key;
    std::string concat;
    size_t hash_;

    location();
    // supports: I. scope:key, II. :key -> def_scope:key, III. key ->
    // def_scope:key
    location(const std::string& cat);
    location(const std::string& sc, const std::string& k);
    location(const char ch_arr[]);

    path find_path();
    operator std::string() const;
    bool operator==(const location& other) const;
    bool operator<(const location& other) const;
};

}  // namespace arc

namespace std {

template <>
struct hash<arc::location> {
    std::size_t operator()(const arc::location& loc) const noexcept { return loc.hash_; }
};

}  // namespace std
