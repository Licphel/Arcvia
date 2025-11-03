#pragma once
#include <format>
#include <iostream>
#include <string_view>

namespace arc {

enum class log_level { info, warn, fatal };

static std::mutex log_mutex_;

void log_redirect(const std::string& logv);
std::string get_header_(log_level type);

template <typename... Args>
void print(log_level type, std::string_view fmt, Args&&... args) {
    std::string formatted = get_header_(type) + std::vformat(fmt, std::make_format_args(args...));
    std::lock_guard<std::mutex> lock(log_mutex_);
    std::cout << formatted << std::endl << std::flush;
    log_redirect(formatted);
}

template <typename... Args>
[[noreturn]] void print_throw(log_level type, std::string_view fmt, Args&&... args) {
    std::string formatted = get_header_(type) + std::vformat(fmt, std::make_format_args(args...));
    std::lock_guard<std::mutex> lock(log_mutex_);
    std::cout << formatted << std::endl << std::flush;
    log_redirect(formatted);
    throw std::runtime_error(formatted);
}

}  // namespace arc
