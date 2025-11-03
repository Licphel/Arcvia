#include "core/loc.h"

#include "core/def.h"

namespace arc {

location::location() = default;

location::location(const std::string& cat) {
    auto pos = cat.find_last_of(':');

    if (pos == std::string::npos) {
        key = cat;
        scope = ARC_LIB_NAME;
        concat = ARC_LIB_NAME + cat;
    } else {
        key = cat.substr(pos + 1);
        scope = cat.substr(0, pos);
        concat = cat;
    }

    hash_ = std::hash<std::string>{}(concat);
}

location::location(const std::string& sc, const std::string& k) {
    scope = sc;
    key = k;
    concat = sc + ":" + key;
    hash_ = std::hash<std::string>{}(concat);
}

location::location(const char ch_arr[]) : location(std::string(ch_arr)) {}

path location::find_path() {
    if (scope == ARC_LIB_NAME) return path::open_local("") / key;
    return {};  // todo: add mod system
}

location::operator std::string() const { return concat; }

bool location::operator==(const location& other) const {
    if (other.hash_ != hash_) return false;
    return other.concat == concat;
}

bool location::operator<(const location& other) const { return other.concat < concat; }

}  // namespace arc
