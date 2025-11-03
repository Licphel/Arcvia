#include "core/uuid.h"

#include <chrono>
#include <cstring>
#include <random>

namespace arc {

bool uuid::operator==(const uuid& other) const { return std::memcmp(bytes, other.bytes, 16) == 0; }

bool uuid::operator<(const uuid& other) const { return std::memcmp(bytes, other.bytes, 16) < 0; }

uuid::operator std::string() const {
    std::string result;
    result.reserve(32);

    const char hex_chars[] = "0123456789abcdef";

    for (uint8_t byte : bytes) {
        result += hex_chars[(byte >> 4) & 0x0F];
        result += hex_chars[byte & 0x0F];
    }

    return result;
}

uuid uuid::empty() { return uuid(); }

uuid uuid::make() {
    uuid u;

    thread_local static std::random_device rd;
    thread_local static std::mt19937_64 rng(rd());
    std::uniform_int_distribution<uint64_t> dist;

    uint64_t part1 = dist(rng);
    uint64_t part2 = dist(rng);

    auto now = std::chrono::steady_clock::now();
    uint64_t timestamp = static_cast<uint64_t>(now.time_since_epoch().count());

    if ((timestamp & 0xFFFF000000000000ULL) == 0) {
        part1 = (part1 & 0x0000FFFFFFFFFFFFULL) | (timestamp << 48);
    } else {
        part1 = timestamp;
    }

    part2 = (part2 & 0xFFFFFFFFFFFF0FFFull) | 0x0000000000004000ull;
    part2 = (part2 & 0x3FFFFFFFFFFFFFFFull) | 0x8000000000000000ull;

    struct uuid_data {
        uint64_t hi;
        uint64_t lo;
    };

    uuid_data data = {part1, part2};
    std::memcpy(&u, &data, 16);

    u.hash_ = std::hash<uint64_t>{}(part1) ^ (std::hash<uint64_t>{}(part2) << 1);

    return u;
}

}  // namespace arc