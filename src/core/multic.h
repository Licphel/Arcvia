#pragma once
#include <algorithm>
#include <functional>
#include <vector>

namespace arc {

template <typename Sig>
class multicall;

template <typename R, typename... Args>
struct multicall<R(Args...)> {
    using handler = std::function<R(Args...)>;
    std::vector<handler> handlers_;

    multicall& operator+=(handler h) {
        if (h) handlers_.push_back(std::move(h));
        return *this;
    }

    multicall& operator-=(const handler& h) {
        auto it = std::find(handlers_.begin(), handlers_.end(), h);
        if (it != handlers_.end()) handlers_.erase(it);
        return *this;
    }

    R operator()(Args... args) const {
        R ret{};
        for (auto& h : handlers_) ret = h(args...);
        return ret;
    }

    void clear() { handlers_.clear(); }
    bool empty() const { return handlers_.empty(); }
};

template <typename... Args>
struct multicall<void(Args...)> {
    using handler = std::function<void(Args...)>;
    std::vector<handler> handlers_;

    multicall& operator+=(handler h) {
        if (h) handlers_.push_back(std::move(h));
        return *this;
    }
    multicall& operator-=(const handler& h) {
        auto it = std::find(handlers_.begin(), handlers_.end(), h);
        if (it != handlers_.end()) handlers_.erase(it);
        return *this;
    }

    void operator()(Args... args) const {
        for (auto& h : handlers_) h(args...);
    }

    void clear() { handlers_.clear(); }
    bool empty() const { return handlers_.empty(); }
};

}  // namespace arc